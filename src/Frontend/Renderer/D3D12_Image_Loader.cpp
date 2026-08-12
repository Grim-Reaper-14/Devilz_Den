#include "D3D12_Image_Loader.hpp"

#include <Windows.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <imgui_internal.h>

#include <cstddef>
#include <cstring>
#include <limits>
#include <vector>

namespace Devilz::Frontend::Renderer
{
using Microsoft::WRL::ComPtr;

D3D12_Image_Loader& D3D12_Image_Loader::Instance() noexcept
{
    static D3D12_Image_Loader loader;
    return loader;
}

bool D3D12_Image_Loader::Load(const std::filesystem::path& path, std::string* error) noexcept
{
    SweepRetired();

    if (!ImGui::GetCurrentContext()) {
        if (error) *error = "ImGui context is not ready for banner textures.";
        return false;
    }

    if (path.empty()) {
        if (error) *error = "Banner image path is empty.";
        return false;
    }

    std::error_code ec;
    if (!std::filesystem::is_regular_file(path, ec) || ec) {
        if (error) *error = "Banner image file does not exist.";
        return false;
    }

    const HRESULT initHr = ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    const bool uninit = SUCCEEDED(initHr);
    if (FAILED(initHr) && initHr != RPC_E_CHANGED_MODE) {
        if (error) *error = "Could not initialize Windows Imaging Component.";
        return false;
    }

    ComPtr<IWICImagingFactory> factory;
    HRESULT hr = ::CoCreateInstance(CLSID_WICImagingFactory2, nullptr, CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(factory.ReleaseAndGetAddressOf()));
    if (FAILED(hr)) {
        hr = ::CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(factory.ReleaseAndGetAddressOf()));
    }
    if (FAILED(hr)) {
        if (uninit) ::CoUninitialize();
        if (error) *error = "Could not create WIC imaging factory.";
        return false;
    }

    ComPtr<IWICBitmapDecoder> decoder;
    const auto wide = path.wstring();
    hr = factory->CreateDecoderFromFilename(wide.c_str(), nullptr, GENERIC_READ,
        WICDecodeMetadataCacheOnLoad, decoder.ReleaseAndGetAddressOf());
    if (FAILED(hr)) {
        if (uninit) ::CoUninitialize();
        if (error) *error = "Unsupported or unreadable image file.";
        return false;
    }

    ComPtr<IWICBitmapFrameDecode> frame;
    hr = decoder->GetFrame(0, frame.ReleaseAndGetAddressOf());
    if (FAILED(hr)) {
        if (uninit) ::CoUninitialize();
        if (error) *error = "Could not decode the first image frame.";
        return false;
    }

    UINT width = 0;
    UINT height = 0;
    if (FAILED(frame->GetSize(&width, &height)) || width == 0 || height == 0 || width > 8192 || height > 8192) {
        if (uninit) ::CoUninitialize();
        if (error) *error = "Banner dimensions are invalid or too large.";
        return false;
    }

    ComPtr<IWICFormatConverter> converter;
    hr = factory->CreateFormatConverter(converter.ReleaseAndGetAddressOf());
    if (FAILED(hr) || FAILED(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA,
        WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom))) {
        if (uninit) ::CoUninitialize();
        if (error) *error = "Could not convert banner to RGBA32.";
        return false;
    }

    const std::size_t rowPitch = static_cast<std::size_t>(width) * 4U;
    const std::size_t byteCount = rowPitch * static_cast<std::size_t>(height);
    if (byteCount > static_cast<std::size_t>((std::numeric_limits<UINT>::max)())) {
        if (uninit) ::CoUninitialize();
        if (error) *error = "Banner image is too large.";
        return false;
    }

    std::vector<unsigned char> pixels(byteCount);
    hr = converter->CopyPixels(nullptr, static_cast<UINT>(rowPitch), static_cast<UINT>(byteCount), pixels.data());
    if (FAILED(hr)) {
        if (uninit) ::CoUninitialize();
        if (error) *error = "Could not copy decoded banner pixels.";
        return false;
    }

    auto texture = std::make_unique<ImTextureData>();
    texture->Create(ImTextureFormat_RGBA32, static_cast<int>(width), static_cast<int>(height));
    if (!texture->Pixels) {
        if (uninit) ::CoUninitialize();
        if (error) *error = "Could not allocate ImGui banner texture.";
        return false;
    }

    std::memcpy(texture->Pixels, pixels.data(), byteCount);
    texture->UseColors = true;
    texture->SetStatus(ImTextureStatus_WantCreate);

    // ImGui 1.92+ only sends registered user textures through PlatformIO.Textures.
    // The DX12 backend consumes that list before drawing and creates the SRV/GPU resource.
    ImGui::RegisterUserTexture(texture.get());

    RetireActive();
    m_active = std::move(texture);
    m_loadedPath = path.string();
    m_width = static_cast<int>(width);
    m_height = static_cast<int>(height);

    if (uninit) ::CoUninitialize();
    return true;
}

void D3D12_Image_Loader::RetireActive() noexcept
{
    if (!m_active)
        return;

    // Let ImGui/DX12 defer destruction until the texture is no longer referenced
    // by any in-flight frame. SweepRetired() unregisters it after the backend
    // reports ImTextureStatus_Destroyed.
    m_active->WantDestroyNextFrame = true;
    m_retired.push_back(std::move(m_active));
}

void D3D12_Image_Loader::SweepRetired() noexcept
{
    if (!ImGui::GetCurrentContext())
        return;

    for (auto it = m_retired.begin(); it != m_retired.end();) {
        auto* texture = it->get();
        if (texture && texture->Status == ImTextureStatus_Destroyed) {
            if (texture->RefCount > 0)
                ImGui::UnregisterUserTexture(texture);
            it = m_retired.erase(it);
        } else {
            ++it;
        }
    }
}

void D3D12_Image_Loader::Tick() noexcept
{
    SweepRetired();
}

void D3D12_Image_Loader::Clear() noexcept
{
    SweepRetired();
    RetireActive();
    m_loadedPath.clear();
    m_width = 0;
    m_height = 0;
}

void D3D12_Image_Loader::Shutdown() noexcept
{
    // This singleton normally outlives the ImGui context. If Shutdown is called
    // while a context is still alive, only unregister textures whose GPU resource
    // has already been destroyed; live resources remain owned until backend shutdown.
    if (ImGui::GetCurrentContext()) {
        SweepRetired();
        if (m_active && m_active->Status == ImTextureStatus_Destroyed && m_active->RefCount > 0)
            ImGui::UnregisterUserTexture(m_active.get());
        for (auto& texture : m_retired) {
            if (texture && texture->Status == ImTextureStatus_Destroyed && texture->RefCount > 0)
                ImGui::UnregisterUserTexture(texture.get());
        }
    }

    m_active.reset();
    m_retired.clear();
    m_loadedPath.clear();
    m_width = 0;
    m_height = 0;
}

bool D3D12_Image_Loader::Ready() const noexcept
{
    return m_active &&
        m_active->Pixels != nullptr &&
        !m_active->WantDestroyNextFrame &&
        m_active->Status != ImTextureStatus_Destroyed;
}

ImTextureRef D3D12_Image_Loader::Texture() noexcept
{
    return m_active ? m_active->GetTexRef() : ImTextureRef{};
}
}
