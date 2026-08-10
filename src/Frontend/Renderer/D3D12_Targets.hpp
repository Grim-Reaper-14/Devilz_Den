#pragma once

#include "Backend/Error/Result.hpp"
#include "Integrations/GTA5_Enhanced/GTA_Module_Manager.hpp"

#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>

#include <cstdint>

namespace Devilz::Frontend
{
struct D3D12_Targets
{
    Microsoft::WRL::ComPtr<IDXGISwapChain3> swapChain;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
    std::uintptr_t signatureAddress = 0;
    std::uintptr_t swapChainStorage = 0;
    std::uintptr_t commandQueueStorage = 0;

    [[nodiscard]] bool Ready() const noexcept
    {
        return swapChain != nullptr && commandQueue != nullptr;
    }
};

class D3D12_Target_Resolver final
{
public:
    static constexpr std::uint64_t SupportedFingerprint = 0x6A4F97F605B81000ULL;
    static constexpr const char* GraphicsPattern = "72 C7 EB 02 31 C0 8B 0D";

    [[nodiscard]] static Backend::Result<D3D12_Targets> Resolve(
        const Integrations::GTA5_Enhanced::GTA_Module_Status& status);
};
}
