#include "D3D12_Image_Loader.hpp"

#include <Windows.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <imgui_internal.h>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <limits>
#include <system_error>
#include <vector>

namespace Devilz::Frontend::Renderer
{
using Microsoft::WRL::ComPtr;

D3D12_Image_Loader& D3D12_Image_Loader::Instance() noexcept
{
    static D3D12_Image_Loader loader;
    return loader;
}

std::string D3D12_Image_Loader::KeyFor(const std::filesystem::path& path)
{
    auto key = path.lexically_normal().generic_string();
    std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return key;
}

bool D3D12_Image_Loader::DecodeAndRegister(
    const std::filesystem::path& path,
    Image_Record& record,
    std::string* error) noexcept
{
    if (!ImGui::GetCurrentContext()) {
        if (error) *error = "ImGui context is not ready for image textures.";
        return false;
    }

    std::error_code ec;
    if (!std::filesystem::is_regular_file(path, ec) || ec) {
        if (error) *error = "Image file does not exist.";
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
        if (error) *error = "Could not decode image frame.";
        return false;
    }

    UINT width = 0;
    UINT height = 0;
    if (FAILED(frame->GetSize(&width, &height)) || width == 0 || height == 0 || width > 8192 || height > 8192) {
        if (uninit) ::CoUninitialize();
        if (error) *error = "Image dimensions are invalid or too large.";
        return false;
    }

    ComPtr<IWICFormatConverter> converter;
    hr = factory->CreateFormatConverter(converter.ReleaseAndGetAddressOf());
    if (FAILED(hr) || FAILED(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA,
        WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom))) {
        if (uninit) ::CoUninitialize();
        if (error) *error = "Could not convert image to RGBA32.";
        return false;
    }

    const std::size_t rowPitch = static_cast<std::size_t>(width) * 4U;
    const std::size_t byteCount = rowPitch * static_cast<std::size_t>(height);
    if (byteCount > static_cast<std::size_t>((std::numeric_limits<UINT>::max)())) {
        if (uninit) ::CoUninitialize();
        if (error) *error = "Image is too large.";
        return false;
    }

    std::vector<unsigned char> pixels(byteCount);
    hr = converter->CopyPixels(nullptr, static_cast<UINT>(rowPitch), static_cast<UINT>(byteCount), pixels.data());
    if (FAILED(hr)) {
        if (uninit) ::CoUninitialize();
        if (error) *error = "Could not copy decoded image pixels.";
        return false;
    }

    auto texture = std::make_unique<ImTextureData>();
    texture->Create(ImTextureFormat_RGBA32, static_cast<int>(width), static_cast<int>(height));
    if (!texture->Pixels) {
        if (uninit) ::CoUninitialize();
        if (error) *error = "Could not allocate ImGui image texture.";
        return false;
    }

    std::memcpy(texture->Pixels, pixels.data(), byteCount);
    texture->UseColors = true;
    texture->SetStatus(ImTextureStatus_WantCreate);
    ImGui::RegisterUserTexture(texture.get());

    record.path = path;
    record.texture = std::move(texture);
    record.width = static_cast<int>(width);
    record.height = static_cast<int>(height);
    record.lastWrite = std::filesystem::last_write_time(path, ec);

    if (uninit) ::CoUninitialize();
    return true;
}

bool D3D12_Image_Loader::EnsureLoaded(const std::filesystem::path& path, std::string* error) noexcept
{
    SweepRetired();
    if (path.empty()) {
        if (error) *error = "Image path is empty.";
        return false;
    }

    const auto key = KeyFor(path);
    auto found = m_images.find(key);
    // Rendering the menu must never poll the filesystem for already-cached
    // textures. Explicit Reload()/Remove() calls handle intentional changes.
    if (found != m_images.end())
        return true;

    Image_Record replacement;
    if (!DecodeAndRegister(path, replacement, error))
        return false;

    m_images.emplace(key, std::move(replacement));
    return true;
}

bool D3D12_Image_Loader::Reload(const std::filesystem::path& path, std::string* error) noexcept
{
    Remove(path);
    return EnsureLoaded(path, error);
}

D3D12_Image_View D3D12_Image_Loader::View(const std::filesystem::path& path) noexcept
{
    D3D12_Image_View view{};
    if (!EnsureLoaded(path, nullptr))
        return view;

    const auto found = m_images.find(KeyFor(path));
    if (found == m_images.end() || !found->second.texture)
        return view;

    const auto& record = found->second;
    view.texture = record.texture->GetTexRef();
    view.width = record.width;
    view.height = record.height;
    view.ready = !record.texture->WantDestroyNextFrame &&
        record.texture->Status != ImTextureStatus_Destroyed;
    return view;
}

void D3D12_Image_Loader::Retire(std::unique_ptr<ImTextureData> texture) noexcept
{
    if (!texture)
        return;
    texture->WantDestroyNextFrame = true;
    m_retired.push_back(std::move(texture));
}

void D3D12_Image_Loader::Remove(const std::filesystem::path& path) noexcept
{
    const auto key = KeyFor(path);
    auto found = m_images.find(key);
    if (found == m_images.end())
        return;
    Retire(std::move(found->second.texture));
    m_images.erase(found);
    if (m_activeKey == key) {
        m_activeKey.clear();
        m_loadedPath.clear();
        m_activeWidth = 0;
        m_activeHeight = 0;
    }
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

bool D3D12_Image_Loader::Load(const std::filesystem::path& path, std::string* error) noexcept
{
    if (!EnsureLoaded(path, error))
        return false;
    const auto key = KeyFor(path);
    const auto found = m_images.find(key);
    if (found == m_images.end())
        return false;
    m_activeKey = key;
    m_loadedPath = path.string();
    m_activeWidth = found->second.width;
    m_activeHeight = found->second.height;
    return true;
}

bool D3D12_Image_Loader::Ready() const noexcept
{
    const auto found = m_images.find(m_activeKey);
    return found != m_images.end() && found->second.texture &&
        !found->second.texture->WantDestroyNextFrame &&
        found->second.texture->Status != ImTextureStatus_Destroyed;
}

ImTextureRef D3D12_Image_Loader::Texture() noexcept
{
    const auto found = m_images.find(m_activeKey);
    return found != m_images.end() && found->second.texture
        ? found->second.texture->GetTexRef()
        : ImTextureRef{};
}

void D3D12_Image_Loader::Clear() noexcept
{
    m_activeKey.clear();
    m_loadedPath.clear();
    m_activeWidth = 0;
    m_activeHeight = 0;
}

void D3D12_Image_Loader::Shutdown() noexcept
{
    if (ImGui::GetCurrentContext()) {
        SweepRetired();
        const auto unregister = [](ImTextureData* texture) {
            if (!texture || texture->RefCount == 0)
                return;
            if (texture->Status == ImTextureStatus_Destroyed ||
                (texture->BackendUserData == nullptr && texture->TexID == ImTextureID_Invalid)) {
                ImGui::UnregisterUserTexture(texture);
            }
        };
        for (auto& [key, record] : m_images) {
            (void)key;
            unregister(record.texture.get());
        }
        for (auto& texture : m_retired)
            unregister(texture.get());
    }

    m_images.clear();
    m_retired.clear();
    m_activeKey.clear();
    m_loadedPath.clear();
    m_activeWidth = 0;
    m_activeHeight = 0;
}
}
