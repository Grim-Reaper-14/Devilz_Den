#include "D3D12_Renderer.hpp"

#include "Backend/Logging/LoggerService.hpp"
#include "Frontend/Menu/Devils_Den_Theme.hpp"

#include <backends/imgui_impl_dx12.h>
#include <backends/imgui_impl_win32.h>
#include <imgui.h>

#include <algorithm>
#include <string>

namespace Devilz::Frontend
{
namespace
{
constexpr UINT SrvDescriptorCapacity = 64;

bool IsMouseMessage(UINT message) noexcept
{
    return message >= WM_MOUSEFIRST && message <= WM_MOUSELAST;
}

bool IsKeyboardMessage(UINT message) noexcept
{
    return (message >= WM_KEYFIRST && message <= WM_KEYLAST) ||
           message == WM_CHAR ||
           message == WM_SYSCHAR;
}
}

std::atomic<D3D12_Renderer*> D3D12_Renderer::s_active{nullptr};

D3D12_Renderer::~D3D12_Renderer()
{
    Shutdown();
}

bool D3D12_Renderer::Initialize(D3D12_Targets targets, Backend::LoggerService& logger) noexcept
{
    if (m_initialized.load())
        return true;
    if (!targets.Ready())
        return false;

    D3D12_Renderer* expected = nullptr;
    if (!s_active.compare_exchange_strong(expected, this))
        return false;

    m_logger = &logger;
    m_targets = std::move(targets);

    if (!CreateDeviceResources()) {
        Shutdown();
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    Devils_Den_Theme::Apply();

    if (!ImGui_ImplWin32_Init(m_window)) {
        Shutdown();
        return false;
    }

    ImGui_ImplDX12_InitInfo initInfo{};
    initInfo.Device = m_device.Get();
    initInfo.CommandQueue = m_targets.commandQueue.Get();
    initInfo.NumFramesInFlight = static_cast<int>(m_frames.size());
    initInfo.RTVFormat = m_backBufferFormat;
    initInfo.DSVFormat = DXGI_FORMAT_UNKNOWN;
    initInfo.UserData = this;
    initInfo.SrvDescriptorHeap = m_srvHeap.Get();
    initInfo.SrvDescriptorAllocFn = &AllocateSrvDescriptor;
    initInfo.SrvDescriptorFreeFn = &FreeSrvDescriptor;

    if (!ImGui_ImplDX12_Init(&initInfo)) {
        Shutdown();
        return false;
    }

    ::SetLastError(ERROR_SUCCESS);
    const auto previous = reinterpret_cast<WNDPROC>(::SetWindowLongPtrW(
        m_window,
        GWLP_WNDPROC,
        reinterpret_cast<LONG_PTR>(&WndProcThunk)));
    if (!previous && ::GetLastError() != ERROR_SUCCESS) {
        Shutdown();
        return false;
    }
    m_originalWndProc = previous;

    m_menuOpen.store(true);
    io.MouseDrawCursor = true;
    m_initialized.store(true);

    if (m_logger) {
        m_logger->Log(
            Backend::LogLevel::Info,
            "D3D12/ImGui renderer initialized | Hotkey: INSERT | InitialPage: Self",
            "GTA5_Enhanced.Frontend");
    }

    return true;
}

void D3D12_Renderer::Shutdown() noexcept
{
    m_initialized.store(false);
    m_resizing.store(true);
    m_menuOpen.store(false);

    if (m_window && m_originalWndProc) {
        ::SetWindowLongPtrW(
            m_window,
            GWLP_WNDPROC,
            reinterpret_cast<LONG_PTR>(m_originalWndProc));
        m_originalWndProc = nullptr;
    }

    if (ImGui::GetCurrentContext()) {
        WaitForAllFrames();
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }

    ReleaseRenderTargets();
    m_frames.clear();
    m_freeSrvDescriptors.clear();
    m_commandList.Reset();
    m_rtvHeap.Reset();
    m_srvHeap.Reset();
    m_fence.Reset();
    m_device.Reset();
    m_targets = {};

    if (m_fenceEvent) {
        ::CloseHandle(m_fenceEvent);
        m_fenceEvent = nullptr;
    }

    m_window = nullptr;
    m_backBufferFormat = DXGI_FORMAT_UNKNOWN;
    m_rtvDescriptorSize = 0;
    m_srvDescriptorSize = 0;
    m_nextFenceValue = 1;
    m_logger = nullptr;

    D3D12_Renderer* expected = this;
    s_active.compare_exchange_strong(expected, nullptr);
}

bool D3D12_Renderer::CreateDeviceResources() noexcept
{
    if (FAILED(m_targets.swapChain->GetDevice(IID_PPV_ARGS(m_device.ReleaseAndGetAddressOf()))))
        return false;

    DXGI_SWAP_CHAIN_DESC desc{};
    if (FAILED(m_targets.swapChain->GetDesc(&desc)) || !desc.OutputWindow || desc.BufferCount == 0)
        return false;

    m_window = desc.OutputWindow;
    m_backBufferFormat = desc.BufferDesc.Format;

    D3D12_DESCRIPTOR_HEAP_DESC rtvDesc{};
    rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvDesc.NumDescriptors = desc.BufferCount;
    if (FAILED(m_device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(m_rtvHeap.ReleaseAndGetAddressOf()))))
        return false;

    D3D12_DESCRIPTOR_HEAP_DESC srvDesc{};
    srvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvDesc.NumDescriptors = SrvDescriptorCapacity;
    srvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (FAILED(m_device->CreateDescriptorHeap(&srvDesc, IID_PPV_ARGS(m_srvHeap.ReleaseAndGetAddressOf()))))
        return false;

    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_srvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    m_frames.resize(desc.BufferCount);
    for (auto& frame : m_frames) {
        if (FAILED(m_device->CreateCommandAllocator(
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                IID_PPV_ARGS(frame.allocator.ReleaseAndGetAddressOf())))) {
            return false;
        }
    }

    if (FAILED(m_device->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            m_frames.front().allocator.Get(),
            nullptr,
            IID_PPV_ARGS(m_commandList.ReleaseAndGetAddressOf())))) {
        return false;
    }
    if (FAILED(m_commandList->Close()))
        return false;

    if (FAILED(m_device->CreateFence(
            0,
            D3D12_FENCE_FLAG_NONE,
            IID_PPV_ARGS(m_fence.ReleaseAndGetAddressOf())))) {
        return false;
    }

    m_fenceEvent = ::CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!m_fenceEvent)
        return false;

    m_freeSrvDescriptors.reserve(SrvDescriptorCapacity);
    for (std::size_t index = SrvDescriptorCapacity; index > 0; --index)
        m_freeSrvDescriptors.push_back(index - 1);

    return CreateRenderTargets();
}

bool D3D12_Renderer::CreateRenderTargets() noexcept
{
    if (!m_rtvHeap || !m_targets.swapChain || m_frames.empty())
        return false;

    auto handle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (UINT index = 0; index < static_cast<UINT>(m_frames.size()); ++index) {
        auto& frame = m_frames[index];
        frame.rtv = handle;
        if (FAILED(m_targets.swapChain->GetBuffer(
                index,
                IID_PPV_ARGS(frame.backBuffer.ReleaseAndGetAddressOf())))) {
            return false;
        }
        m_device->CreateRenderTargetView(frame.backBuffer.Get(), nullptr, frame.rtv);
        handle.ptr += m_rtvDescriptorSize;
    }
    return true;
}

void D3D12_Renderer::ReleaseRenderTargets() noexcept
{
    for (auto& frame : m_frames)
        frame.backBuffer.Reset();
}

void D3D12_Renderer::WaitForFrame(Frame_Context& frame) noexcept
{
    if (!m_fence || !m_fenceEvent || frame.fenceValue == 0)
        return;
    if (m_fence->GetCompletedValue() >= frame.fenceValue) {
        frame.fenceValue = 0;
        return;
    }

    if (SUCCEEDED(m_fence->SetEventOnCompletion(frame.fenceValue, m_fenceEvent)))
        ::WaitForSingleObject(m_fenceEvent, INFINITE);
    frame.fenceValue = 0;
}

void D3D12_Renderer::WaitForAllFrames() noexcept
{
    for (auto& frame : m_frames)
        WaitForFrame(frame);
}

void D3D12_Renderer::OnPresent() noexcept
{
    if (!m_initialized.load(std::memory_order_acquire) ||
        m_resizing.load(std::memory_order_acquire) ||
        !ImGui::GetCurrentContext()) {
        return;
    }

    const auto index = m_targets.swapChain->GetCurrentBackBufferIndex();
    if (index >= m_frames.size())
        return;

    auto& frame = m_frames[index];
    WaitForFrame(frame);

    if (FAILED(frame.allocator->Reset()))
        return;
    if (FAILED(m_commandList->Reset(frame.allocator.Get(), nullptr)))
        return;

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    bool open = m_menuOpen.load();
    m_menu.Draw(open);
    m_menuOpen.store(open);
    ImGui::GetIO().MouseDrawCursor = open;

    ImGui::Render();

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = frame.backBuffer.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    m_commandList->ResourceBarrier(1, &barrier);
    m_commandList->OMSetRenderTargets(1, &frame.rtv, FALSE, nullptr);

    ID3D12DescriptorHeap* heaps[] = {m_srvHeap.Get()};
    m_commandList->SetDescriptorHeaps(1, heaps);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_commandList.Get());

    std::swap(barrier.Transition.StateBefore, barrier.Transition.StateAfter);
    m_commandList->ResourceBarrier(1, &barrier);

    if (FAILED(m_commandList->Close()))
        return;

    ID3D12CommandList* lists[] = {m_commandList.Get()};
    m_targets.commandQueue->ExecuteCommandLists(1, lists);

    const auto fenceValue = m_nextFenceValue++;
    if (SUCCEEDED(m_targets.commandQueue->Signal(m_fence.Get(), fenceValue)))
        frame.fenceValue = fenceValue;
}

void D3D12_Renderer::BeforeResize() noexcept
{
    if (!m_initialized.load())
        return;

    m_resizing.store(true);
    WaitForAllFrames();
    if (ImGui::GetCurrentContext())
        ImGui_ImplDX12_InvalidateDeviceObjects();
    ReleaseRenderTargets();
}

void D3D12_Renderer::AfterResize() noexcept
{
    if (!m_initialized.load())
        return;

    DXGI_SWAP_CHAIN_DESC desc{};
    if (FAILED(m_targets.swapChain->GetDesc(&desc)) || desc.BufferCount != m_frames.size()) {
        if (m_logger) {
            m_logger->Log(
                Backend::LogLevel::Warning,
                "D3D12 resize changed swap-chain buffer count; frontend rendering paused",
                "GTA5_Enhanced.Frontend");
        }
        return;
    }

    m_backBufferFormat = desc.BufferDesc.Format;
    if (!CreateRenderTargets())
        return;

    if (ImGui::GetCurrentContext() && !ImGui_ImplDX12_CreateDeviceObjects())
        return;

    m_resizing.store(false);
}

void D3D12_Renderer::AllocateSrvDescriptor(
    ImGui_ImplDX12_InitInfo* info,
    D3D12_CPU_DESCRIPTOR_HANDLE* cpuHandle,
    D3D12_GPU_DESCRIPTOR_HANDLE* gpuHandle)
{
    if (!info || !info->UserData || !cpuHandle || !gpuHandle)
        return;

    auto* renderer = static_cast<D3D12_Renderer*>(info->UserData);
    std::scoped_lock lock(renderer->m_srvMutex);
    if (renderer->m_freeSrvDescriptors.empty()) {
        *cpuHandle = {};
        *gpuHandle = {};
        return;
    }

    const auto index = renderer->m_freeSrvDescriptors.back();
    renderer->m_freeSrvDescriptors.pop_back();

    *cpuHandle = renderer->m_srvHeap->GetCPUDescriptorHandleForHeapStart();
    cpuHandle->ptr += index * renderer->m_srvDescriptorSize;
    *gpuHandle = renderer->m_srvHeap->GetGPUDescriptorHandleForHeapStart();
    gpuHandle->ptr += index * renderer->m_srvDescriptorSize;
}

void D3D12_Renderer::FreeSrvDescriptor(
    ImGui_ImplDX12_InitInfo* info,
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
    D3D12_GPU_DESCRIPTOR_HANDLE)
{
    if (!info || !info->UserData || cpuHandle.ptr == 0)
        return;

    auto* renderer = static_cast<D3D12_Renderer*>(info->UserData);
    const auto start = renderer->m_srvHeap->GetCPUDescriptorHandleForHeapStart().ptr;
    if (cpuHandle.ptr < start || renderer->m_srvDescriptorSize == 0)
        return;

    const auto delta = cpuHandle.ptr - start;
    if ((delta % renderer->m_srvDescriptorSize) != 0)
        return;

    const auto index = static_cast<std::size_t>(delta / renderer->m_srvDescriptorSize);
    if (index >= SrvDescriptorCapacity)
        return;

    std::scoped_lock lock(renderer->m_srvMutex);
    if (std::find(renderer->m_freeSrvDescriptors.begin(), renderer->m_freeSrvDescriptors.end(), index) ==
        renderer->m_freeSrvDescriptors.end()) {
        renderer->m_freeSrvDescriptors.push_back(index);
    }
}

LRESULT CALLBACK D3D12_Renderer::WndProcThunk(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto* renderer = s_active.load(std::memory_order_acquire);
    if (!renderer)
        return ::DefWindowProcW(window, message, wParam, lParam);
    return renderer->WndProc(window, message, wParam, lParam);
}

LRESULT D3D12_Renderer::WndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) noexcept
{
    if (message == WM_KEYUP && wParam == VK_INSERT) {
        const bool next = !m_menuOpen.load();
        m_menuOpen.store(next);
        if (ImGui::GetCurrentContext())
            ImGui::GetIO().MouseDrawCursor = next;
        return 0;
    }

    if (ImGui::GetCurrentContext()) {
        const auto handled = ImGui_ImplWin32_WndProcHandler(window, message, wParam, lParam);
        if (m_menuOpen.load()) {
            const auto& io = ImGui::GetIO();
            if (handled || (io.WantCaptureMouse && IsMouseMessage(message)) ||
                (io.WantCaptureKeyboard && IsKeyboardMessage(message))) {
                return 1;
            }
        }
    }

    return m_originalWndProc
        ? ::CallWindowProcW(m_originalWndProc, window, message, wParam, lParam)
        : ::DefWindowProcW(window, message, wParam, lParam);
}
}
