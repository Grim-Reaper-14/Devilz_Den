#pragma once

#include "D3D12_Targets.hpp"
#include "Frontend/Menu/Devils_Den_Menu.hpp"

#include <Windows.h>
#include <d3d12.h>
#include <wrl/client.h>
#include <imgui.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

struct ImGui_ImplDX12_InitInfo;

namespace Devilz::Backend
{
class LoggerService;
}

namespace Devilz::Frontend
{
class Viewport_Devils_Den_Menu final
{
public:
    void Draw(bool& open)
    {
        if (open && !m_wasOpen) {
            if (const auto* viewport = ImGui::GetMainViewport()) {
                constexpr float desiredMenuWidth = 1320.0F;
                constexpr float topMargin = 12.0F;
                float xOffset = (viewport->WorkSize.x - desiredMenuWidth) * 0.5F;
                if (xOffset < topMargin)
                    xOffset = topMargin;

                ImGui::SetNextWindowPos(
                    ImVec2{viewport->WorkPos.x + xOffset, viewport->WorkPos.y + topMargin},
                    ImGuiCond_Always);
            }
        }

        m_menu.Draw(open);
        m_wasOpen = open;
    }

private:
    Devils_Den_Menu m_menu;
    bool m_wasOpen = false;
};

class D3D12_Renderer final
{
public:
    D3D12_Renderer() = default;
    ~D3D12_Renderer();

    D3D12_Renderer(const D3D12_Renderer&) = delete;
    D3D12_Renderer& operator=(const D3D12_Renderer&) = delete;

    [[nodiscard]] bool Initialize(D3D12_Targets targets, Backend::LoggerService& logger) noexcept;
    void Shutdown() noexcept;

    void OnPresent() noexcept;
    void BeforeResize() noexcept;
    void AfterResize() noexcept;

    [[nodiscard]] bool Ready() const noexcept { return m_initialized.load(); }
    [[nodiscard]] bool MenuOpen() const noexcept { return m_menuOpen.load(); }

private:
    struct Frame_Context
    {
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
        Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer;
        D3D12_CPU_DESCRIPTOR_HANDLE rtv{};
        std::uint64_t fenceValue = 0;
    };

    [[nodiscard]] bool CreateDeviceResources() noexcept;
    [[nodiscard]] bool CreateRenderTargets() noexcept;
    void ReleaseRenderTargets() noexcept;
    void WaitForFrame(Frame_Context& frame) noexcept;
    void WaitForAllFrames() noexcept;

    static void AllocateSrvDescriptor(
        ImGui_ImplDX12_InitInfo* info,
        D3D12_CPU_DESCRIPTOR_HANDLE* cpuHandle,
        D3D12_GPU_DESCRIPTOR_HANDLE* gpuHandle);
    static void FreeSrvDescriptor(
        ImGui_ImplDX12_InitInfo* info,
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle);

    static LRESULT CALLBACK WndProcThunk(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT WndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) noexcept;

    Backend::LoggerService* m_logger = nullptr;
    D3D12_Targets m_targets{};
    Microsoft::WRL::ComPtr<ID3D12Device> m_device;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
    Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
    std::vector<Frame_Context> m_frames;
    std::vector<std::size_t> m_freeSrvDescriptors;
    std::mutex m_srvMutex;
    HANDLE m_fenceEvent = nullptr;
    HWND m_window = nullptr;
    WNDPROC m_originalWndProc = nullptr;
    DXGI_FORMAT m_backBufferFormat = DXGI_FORMAT_UNKNOWN;
    UINT m_rtvDescriptorSize = 0;
    UINT m_srvDescriptorSize = 0;
    std::uint64_t m_nextFenceValue = 1;
    Viewport_Devils_Den_Menu m_menu;
    bool m_win32BackendInitialized = false;
    bool m_dx12BackendInitialized = false;
    std::atomic_bool m_initialized{false};
    std::atomic_bool m_resizing{false};
    std::atomic_bool m_menuOpen{false};

    static std::atomic<D3D12_Renderer*> s_active;
};
}
