#include "GTA_Network_Session_Extension.hpp"

#include "GTA_Network_Session_State.hpp"
#include "GTA_Script_Function_Invoker.hpp"
#include "Backend/Logging/LoggerService.hpp"

#include <Windows.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>

namespace Devilz::Integrations::GTA5_Enhanced
{
namespace
{
constexpr std::uint64_t SupportedBuildFingerprint = 0x6A4F97F605B81000ULL;
constexpr std::uint32_t JoinTypeGlobalIndex = 1575048U;
constexpr std::string_view SendToCloudsDescriptorId =
    "network.shop_controller.send_to_clouds";
constexpr std::array<std::int16_t, 11> SendToCloudsPattern{
    0x2D, 0x00, 0x02, 0x00, 0x00, 0x72, 0x5D, -1, -1, -1, 0x72
};

struct NetworkSessionRuntime
{
    std::uintptr_t scriptGlobalsAddress = 0;
    std::uint64_t buildFingerprint = 0;
    Backend::LoggerService* logger = nullptr;
    bool ready = false;
};

NetworkSessionRuntime g_runtime{};

bool IsReadableAddress(std::uintptr_t address, std::size_t size) noexcept
{
    if (address == 0 || size == 0)
        return false;

    MEMORY_BASIC_INFORMATION memory{};
    if (::VirtualQuery(reinterpret_cast<const void*>(address), &memory, sizeof(memory)) == 0)
        return false;
    if (memory.State != MEM_COMMIT || (memory.Protect & (PAGE_GUARD | PAGE_NOACCESS)) != 0)
        return false;

    const DWORD protection = memory.Protect & 0xFFU;
    const bool readable = protection == PAGE_READONLY ||
                          protection == PAGE_READWRITE ||
                          protection == PAGE_WRITECOPY ||
                          protection == PAGE_EXECUTE_READ ||
                          protection == PAGE_EXECUTE_READWRITE ||
                          protection == PAGE_EXECUTE_WRITECOPY;
    if (!readable)
        return false;

    const auto start = reinterpret_cast<std::uintptr_t>(memory.BaseAddress);
    if (start > (std::numeric_limits<std::uintptr_t>::max)() - memory.RegionSize)
        return false;
    const auto end = start + memory.RegionSize;
    return address >= start && address <= end && size <= end - address;
}

bool IsWritableAddress(std::uintptr_t address, std::size_t size) noexcept
{
    if (!IsReadableAddress(address, size))
        return false;

    MEMORY_BASIC_INFORMATION memory{};
    if (::VirtualQuery(reinterpret_cast<const void*>(address), &memory, sizeof(memory)) == 0)
        return false;

    const DWORD protection = memory.Protect & 0xFFU;
    return protection == PAGE_READWRITE || protection == PAGE_WRITECOPY ||
           protection == PAGE_EXECUTE_READWRITE || protection == PAGE_EXECUTE_WRITECOPY;
}

GTA_Script_Function_Descriptor SendToCloudsDescriptor()
{
    GTA_Script_Function_Descriptor descriptor{};
    descriptor.id = std::string{SendToCloudsDescriptorId};
    descriptor.label = "Shop Controller: Send To Clouds";
    descriptor.category = "Network/Session";
    descriptor.scriptName = "shop_controller";
    descriptor.sourceReference =
        "decompileScripts/scripts/shop_controller.c | verified send-to-clouds entry signature";
    descriptor.source.repository = "acidlabsdev/gtav-enhanced-scripts";
    descriptor.source.revision = "30dd0df8bce87bdc21103555bae380be9fc0a916";
    descriptor.source.path = "scripts/shop_controller.c";
    descriptor.expectedBuildFingerprint = SupportedBuildFingerprint;
    descriptor.verifiedSynchronous = true;
    descriptor.allowNonIdleCompletion = true;
    descriptor.exposure = GTA_Script_Function_Exposure::Internal;
    descriptor.locator = GTA_Script_Function_Locator_Kind::UniqueBytePattern;
    descriptor.bytePattern.assign(SendToCloudsPattern.begin(), SendToCloudsPattern.end());
    descriptor.abi.returnType = GTA_Script_Value_Type::Void;
    return descriptor;
}

bool CallSendToClouds() noexcept
{
    const auto result = InvokeRegisteredScriptFunctionOnGameThread(SendToCloudsDescriptorId);
    return result.status == GTA_Script_Function_Invoke_Status::Executed;
}

int* ResolveJoinTypeGlobal() noexcept
{
    constexpr std::uint32_t blockIndex = (JoinTypeGlobalIndex >> 0x12U) & 0x3FU;
    constexpr std::uint32_t slotIndex = JoinTypeGlobalIndex & 0x3FFFFU;

    if (!IsReadableAddress(g_runtime.scriptGlobalsAddress, 64 * sizeof(std::int64_t*)))
        return nullptr;

    auto** globals = reinterpret_cast<std::int64_t**>(g_runtime.scriptGlobalsAddress);
    std::int64_t* block = nullptr;
    std::memcpy(&block, globals + blockIndex, sizeof(block));
    if (!block)
        return nullptr;

    auto* destination = reinterpret_cast<int*>(block + slotIndex);
    if (!IsWritableAddress(reinterpret_cast<std::uintptr_t>(destination), sizeof(int)))
        return nullptr;
    return destination;
}

bool LaunchJoinType(GTA_Network_Join_Type type) noexcept
{
    if (!g_runtime.ready || !CallSendToClouds())
        return false;

    auto* joinType = ResolveJoinTypeGlobal();
    if (!joinType)
        return false;

    *joinType = static_cast<int>(type);
    return true;
}
}

void ConfigureNetworkSessionExtension(
    std::uintptr_t scriptGlobalsAddress,
    std::uint64_t buildFingerprint,
    Backend::LoggerService* logger) noexcept
{
    g_runtime = {};
    g_runtime.scriptGlobalsAddress = scriptGlobalsAddress;
    g_runtime.buildFingerprint = buildFingerprint;
    g_runtime.logger = logger;

    const bool supportedBuild = buildFingerprint == SupportedBuildFingerprint;
    const bool descriptorReady = supportedBuild && ScriptFunctionInvokerReady() &&
        RegisterScriptFunctionDescriptor(SendToCloudsDescriptor());
    g_runtime.ready = scriptGlobalsAddress != 0 && descriptorReady;
    GTA_Network_Session_State::Instance().SetRuntimeReady(g_runtime.ready);

    if (logger) {
        logger->Log(
            g_runtime.ready ? Backend::LogLevel::Info : Backend::LogLevel::Warning,
            g_runtime.ready
                ? "Network session switcher ready | verified shop_controller descriptor routed through shared ScriptVM engine"
                : "Network session switcher unavailable: unsupported build, ScriptGlobals, or verified ScriptVM engine missing",
            "GTA5_Enhanced.Network");
    }
}

void ResetNetworkSessionExtension() noexcept
{
    GTA_Network_Session_State::Instance().SetRuntimeReady(false);
    g_runtime = {};
}

void TickNetworkSessionExtension() noexcept
{
    auto request = GTA_Network_Session_State::Instance().TakePending();
    if (!request)
        return;

    const bool success = LaunchJoinType(*request);
    GTA_Network_Session_State::Instance().Complete(success);

    if (g_runtime.logger) {
        g_runtime.logger->Log(
            success ? Backend::LogLevel::Info : Backend::LogLevel::Warning,
            success
                ? "Session transition requested through verified shop_controller ScriptVM descriptor"
                : "Session transition request failed before GTA transition launch",
            "GTA5_Enhanced.Network");
    }
}
}
