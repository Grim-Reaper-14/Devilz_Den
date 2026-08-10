#include "D3D12_Present_Hook.hpp"

#include "Backend/Error/Error.hpp"
#include "Frontend/Renderer/D3D12_Renderer.hpp"

#include <Windows.h>

#include <cstring>

namespace Devilz::Integrations::GTA5_Enhanced
{
using namespace Devilz::Backend;

std::atomic<D3D12_Present_Hook*> D3D12_Present_Hook::s_active{nullptr};

D3D12_Present_Hook::D3D12_Present_Hook(
    IDXGISwapChain3* swapChain,
    Frontend::D3D12_Renderer& renderer) noexcept
    : m_swapChain(swapChain), m_renderer(&renderer)
{
}

D3D12_Present_Hook::~D3D12_Present_Hook()
{
    Remove();
}

Result<void> D3D12_Present_Hook::Install()
{
    if (m_state.load() == Hook_State::Installed)
        return Result<void>::Success();

    if (!m_swapChain || !m_renderer) {
        m_state.store(Hook_State::Failed);
        return Result<void>::Failure(Error(
            ErrorCode::InvalidArgument,
            ErrorCategory::Graphics,
            "D3D12 Present hook requires a valid swap chain and renderer"));
    }

    D3D12_Present_Hook* expected = nullptr;
    if (!s_active.compare_exchange_strong(expected, this)) {
        m_state.store(Hook_State::Failed);
        return Result<void>::Failure(Error(
            ErrorCode::RuntimeFailure,
            ErrorCategory::Graphics,
            "Another D3D12 Present hook is already active"));
    }

    m_state.store(Hook_State::Installing);

    m_originalVTable = *reinterpret_cast<void***>(m_swapChain);
    if (!m_originalVTable) {
        s_active.store(nullptr);
        m_state.store(Hook_State::Failed);
        return Result<void>::Failure(Error(
            ErrorCode::D3D12Failure,
            ErrorCategory::Graphics,
            "DXGI swap chain exposes a null virtual table"));
    }

    std::memcpy(
        m_hookVTable.data(),
        m_originalVTable,
        sizeof(void*) * m_hookVTable.size());

    m_originalPresent = reinterpret_cast<PresentFn>(m_hookVTable[PresentIndex]);
    m_originalResizeBuffers = reinterpret_cast<ResizeBuffersFn>(m_hookVTable[ResizeBuffersIndex]);
    if (!m_originalPresent || !m_originalResizeBuffers) {
        s_active.store(nullptr);
        m_state.store(Hook_State::Failed);
        return Result<void>::Failure(Error(
            ErrorCode::D3D12Failure,
            ErrorCategory::Graphics,
            "DXGI swap chain virtual table does not expose Present/ResizeBuffers"));
    }

    m_hookVTable[PresentIndex] = reinterpret_cast<void*>(&PresentThunk);
    m_hookVTable[ResizeBuffersIndex] = reinterpret_cast<void*>(&ResizeBuffersThunk);

    auto*** objectVTable = reinterpret_cast<void***>(m_swapChain);
    ::InterlockedExchangePointer(
        reinterpret_cast<PVOID volatile*>(objectVTable),
        m_hookVTable.data());

    m_state.store(Hook_State::Installed);
    return Result<void>::Success();
}

Result<void> D3D12_Present_Hook::Remove()
{
    const auto state = m_state.load();
    if (state == Hook_State::Created || state == Hook_State::Removed)
        return Result<void>::Success();

    m_state.store(Hook_State::Removing);

    if (m_swapChain && m_originalVTable) {
        auto*** objectVTable = reinterpret_cast<void***>(m_swapChain);
        ::InterlockedCompareExchangePointer(
            reinterpret_cast<PVOID volatile*>(objectVTable),
            m_originalVTable,
            m_hookVTable.data());
    }

    D3D12_Present_Hook* expected = this;
    s_active.compare_exchange_strong(expected, nullptr);

    while (m_activeCalls.load(std::memory_order_acquire) != 0)
        ::Sleep(0);

    m_state.store(Hook_State::Removed);
    return Result<void>::Success();
}

HRESULT STDMETHODCALLTYPE D3D12_Present_Hook::PresentThunk(
    IDXGISwapChain* swapChain,
    UINT syncInterval,
    UINT flags)
{
    auto* hook = s_active.load(std::memory_order_acquire);
    if (!hook || !hook->m_originalPresent)
        return DXGI_ERROR_INVALID_CALL;

    hook->m_activeCalls.fetch_add(1, std::memory_order_acq_rel);
    if (hook->m_state.load(std::memory_order_acquire) == Hook_State::Installed && hook->m_renderer)
        hook->m_renderer->OnPresent();

    const auto result = hook->m_originalPresent(swapChain, syncInterval, flags);
    hook->m_activeCalls.fetch_sub(1, std::memory_order_acq_rel);
    return result;
}

HRESULT STDMETHODCALLTYPE D3D12_Present_Hook::ResizeBuffersThunk(
    IDXGISwapChain* swapChain,
    UINT bufferCount,
    UINT width,
    UINT height,
    DXGI_FORMAT format,
    UINT flags)
{
    auto* hook = s_active.load(std::memory_order_acquire);
    if (!hook || !hook->m_originalResizeBuffers)
        return DXGI_ERROR_INVALID_CALL;

    hook->m_activeCalls.fetch_add(1, std::memory_order_acq_rel);
    if (hook->m_state.load(std::memory_order_acquire) == Hook_State::Installed && hook->m_renderer)
        hook->m_renderer->BeforeResize();

    const auto result = hook->m_originalResizeBuffers(
        swapChain,
        bufferCount,
        width,
        height,
        format,
        flags);

    if (SUCCEEDED(result) &&
        hook->m_state.load(std::memory_order_acquire) == Hook_State::Installed &&
        hook->m_renderer) {
        hook->m_renderer->AfterResize();
    }

    hook->m_activeCalls.fetch_sub(1, std::memory_order_acq_rel);
    return result;
}
}
