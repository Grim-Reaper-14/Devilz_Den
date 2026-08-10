#pragma once

#include "Hook.hpp"

#include <dxgi1_4.h>

#include <array>
#include <atomic>
#include <cstddef>

namespace Devilz::Frontend
{
class D3D12_Renderer;
}

namespace Devilz::Integrations::GTA5_Enhanced
{
class D3D12_Present_Hook final : public Hook
{
public:
    D3D12_Present_Hook(IDXGISwapChain3* swapChain, Frontend::D3D12_Renderer& renderer) noexcept;
    ~D3D12_Present_Hook() override;

    [[nodiscard]] std::string_view Name() const noexcept override { return "D3D12 Present"; }
    [[nodiscard]] Hook_State State() const noexcept override { return m_state.load(); }

    Backend::Result<void> Install() override;
    Backend::Result<void> Remove() override;

private:
    using PresentFn = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT);
    using ResizeBuffersFn = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);

    static constexpr std::size_t VTableEntries = 40;
    static constexpr std::size_t PresentIndex = 8;
    static constexpr std::size_t ResizeBuffersIndex = 13;

    static HRESULT STDMETHODCALLTYPE PresentThunk(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags);
    static HRESULT STDMETHODCALLTYPE ResizeBuffersThunk(
        IDXGISwapChain* swapChain,
        UINT bufferCount,
        UINT width,
        UINT height,
        DXGI_FORMAT format,
        UINT flags);

    IDXGISwapChain3* m_swapChain = nullptr;
    Frontend::D3D12_Renderer* m_renderer = nullptr;
    void** m_originalVTable = nullptr;
    std::array<void*, VTableEntries> m_hookVTable{};
    PresentFn m_originalPresent = nullptr;
    ResizeBuffersFn m_originalResizeBuffers = nullptr;
    std::atomic<Hook_State> m_state{Hook_State::Created};
    std::atomic_uint32_t m_activeCalls{0};

    static std::atomic<D3D12_Present_Hook*> s_active;
    static PresentFn s_fallbackPresent;
    static ResizeBuffersFn s_fallbackResizeBuffers;
};
}
