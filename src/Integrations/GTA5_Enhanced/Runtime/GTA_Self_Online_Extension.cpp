#include "GTA_Self_Online_Extension.hpp"

#include "GTA_Gameplay_State.hpp"
#include "Backend/Logging/LoggerService.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Manager.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Registry.hpp"

#include <Windows.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string_view>

namespace Devilz::Integrations::GTA5_Enhanced
{
namespace
{
constexpr GTA_Native_Hash GetNetworkTimeHash = 0x7E3F74F641EE6B27ULL;
constexpr GTA_Native_Hash StopCutsceneImmediatelyHash = 0xA7E4AA8D29D3DAC1ULL;

constexpr std::uint32_t GlobalPlayerBdBase = 2658296U;
constexpr std::uint32_t GlobalPlayerEntrySize = 468U;
constexpr std::uint32_t GlobalPlayerOffRadarOffset = 214U;
constexpr std::uint32_t OffRadarTimerGlobal = 2673276U + 58U;
constexpr int FreemodeRunningState = 4;

struct SelfOnlineRuntime
{
    std::uintptr_t scriptGlobalsAddress = 0;
    std::uintptr_t scriptThreadsStorageAddress = 0;
    Backend::LoggerService* logger = nullptr;
};

SelfOnlineRuntime g_runtime{};
bool g_offRadarApplied = false;

struct GTA_Script_Context_View
{
    std::uint32_t threadId = 0;
    std::uint32_t pad04 = 0;
    std::uint64_t scriptHash = 0;
    std::int32_t state = 0;
    std::uint32_t programCounter = 0;
    std::uint32_t framePointer = 0;
    std::uint32_t stackPointer = 0;
    float timerA = 0.0F;
    float timerB = 0.0F;
    float waitTimer = 0.0F;
    std::byte pad2C[0x2C]{};
    std::uint32_t stackSize = 0;
    std::byte pad5C[0x54]{};
};

static_assert(sizeof(GTA_Script_Context_View) == 0xB0);

struct GTA_Script_Thread_View
{
    void* vtable = nullptr;
    GTA_Script_Context_View context{};
    std::uint64_t* stack = nullptr;
    std::byte padC0[0x90]{};
    std::uint32_t scriptHash = 0;
};

static_assert(offsetof(GTA_Script_Thread_View, scriptHash) == 0x150);

struct GTA_Script_Thread_Array_View
{
    GTA_Script_Thread_View** data = nullptr;
    std::uint16_t size = 0;
    std::uint16_t capacity = 0;
    std::uint32_t pad0C = 0;
};

static_assert(sizeof(GTA_Script_Thread_Array_View) == 0x10);

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

std::uint32_t Joaat(std::string_view value) noexcept
{
    std::uint32_t hash = 0;
    for (unsigned char character : value) {
        character = static_cast<unsigned char>(std::tolower(character));
        hash += character;
        hash += hash << 10U;
        hash ^= hash >> 6U;
    }
    hash += hash << 3U;
    hash ^= hash >> 11U;
    hash += hash << 15U;
    return hash;
}

GTA_Script_Thread_View* FindFreemodeThread() noexcept
{
    if (!IsReadableAddress(g_runtime.scriptThreadsStorageAddress, sizeof(GTA_Script_Thread_Array_View)))
        return nullptr;

    GTA_Script_Thread_Array_View threads{};
    std::memcpy(&threads,
                reinterpret_cast<const void*>(g_runtime.scriptThreadsStorageAddress),
                sizeof(threads));

    const auto count = std::min<std::size_t>(threads.size, 256);
    if (!threads.data || count == 0 ||
        !IsReadableAddress(reinterpret_cast<std::uintptr_t>(threads.data), count * sizeof(void*))) {
        return nullptr;
    }

    const auto freemodeHash = Joaat("freemode");
    for (std::size_t index = 0; index < count; ++index) {
        GTA_Script_Thread_View* thread = nullptr;
        std::memcpy(&thread, threads.data + index, sizeof(thread));
        if (!thread || !IsReadableAddress(reinterpret_cast<std::uintptr_t>(thread), 0x154))
            continue;
        if (thread->context.threadId != 0 && thread->scriptHash == freemodeHash)
            return thread;
    }

    return nullptr;
}

template <typename T>
T* ResolveGlobal(std::uint32_t index) noexcept
{
    const std::uint32_t blockIndex = (index >> 0x12U) & 0x3FU;
    const std::uint32_t slotIndex = index & 0x3FFFFU;

    if (!IsReadableAddress(g_runtime.scriptGlobalsAddress, 64 * sizeof(std::int64_t*)))
        return nullptr;

    auto** globals = reinterpret_cast<std::int64_t**>(g_runtime.scriptGlobalsAddress);
    std::int64_t* block = nullptr;
    std::memcpy(&block, globals + blockIndex, sizeof(block));
    if (!block)
        return nullptr;

    auto* value = reinterpret_cast<T*>(block + slotIndex);
    if (!IsWritableAddress(reinterpret_cast<std::uintptr_t>(value), sizeof(T)))
        return nullptr;
    return value;
}

std::uint32_t PlayerEntryGlobal(int player, std::uint32_t fieldOffset) noexcept
{
    // GlobalPlayerBD starts with SCR_ARRAY.Size, so Entries[0] begins one slot
    // after the base global. YimMenuV2's Enhanced entry size is 468 slots.
    return GlobalPlayerBdBase + 1U +
           static_cast<std::uint32_t>(player) * GlobalPlayerEntrySize +
           fieldOffset;
}

bool SafeToModifyFreemodeGlobals(int player) noexcept
{
    if (player < 0 || player >= 32 || !FindFreemodeThread())
        return false;

    auto* freemodeState = ResolveGlobal<int>(PlayerEntryGlobal(player, 0U));
    return freemodeState && *freemodeState == FreemodeRunningState;
}

void TickOffTheRadar(GTA_Native_Manager& natives) noexcept
{
    auto& state = GTA_Gameplay_State::Instance();
    const bool desired = state.OffTheRadar();
    if (!desired && !g_offRadarApplied)
        return;

    const auto player = natives.Invoke<int>(GTA_Native_Id::PlayerId);
    if (!player || *player < 0 || *player >= 32)
        return;

    if (!SafeToModifyFreemodeGlobals(*player))
        return;

    auto* active = ResolveGlobal<int>(PlayerEntryGlobal(*player, GlobalPlayerOffRadarOffset));
    if (!active)
        return;

    if (!desired) {
        *active = 0;
        g_offRadarApplied = false;
        if (g_runtime.logger)
            g_runtime.logger->Log(Backend::LogLevel::Info,
                                  "Off The Radar disabled; freemode broadcast flag restored",
                                  "GTA5_Enhanced.Features");
        return;
    }

    const auto networkTime = natives.InvokeHash<int>(GetNetworkTimeHash);
    auto* timer = ResolveGlobal<int>(OffRadarTimerGlobal);
    if (!networkTime || !timer)
        return;

    *timer = *networkTime;
    *active = 1;

    if (!g_offRadarApplied && g_runtime.logger)
        g_runtime.logger->Log(Backend::LogLevel::Info,
                              "Off The Radar enabled through freemode broadcast globals",
                              "GTA5_Enhanced.Features");
    g_offRadarApplied = true;
}

void TickSkipCutscene(GTA_Native_Manager& natives) noexcept
{
    auto& state = GTA_Gameplay_State::Instance();
    if (!state.ConsumeSkipCutsceneRequest())
        return;

    const bool success = natives.InvokeHash<void>(StopCutsceneImmediatelyHash);
    if (g_runtime.logger) {
        g_runtime.logger->Log(
            success ? Backend::LogLevel::Info : Backend::LogLevel::Warning,
            success ? "Skip Cutscene requested" : "Skip Cutscene native invocation failed",
            "GTA5_Enhanced.Features");
    }
}
}

void ConfigureSelfOnlineExtension(
    std::uintptr_t scriptGlobalsAddress,
    std::uintptr_t scriptThreadsStorageAddress,
    Backend::LoggerService* logger) noexcept
{
    g_runtime = {};
    g_runtime.scriptGlobalsAddress = scriptGlobalsAddress;
    g_runtime.scriptThreadsStorageAddress = scriptThreadsStorageAddress;
    g_runtime.logger = logger;
    g_offRadarApplied = false;

    if (logger && (scriptGlobalsAddress == 0 || scriptThreadsStorageAddress == 0)) {
        logger->Log(Backend::LogLevel::Warning,
                    "Off The Radar unavailable: ScriptGlobals or ScriptThreads target missing",
                    "GTA5_Enhanced.Features");
    }
}

void ResetSelfOnlineExtension() noexcept
{
    g_offRadarApplied = false;
    g_runtime = {};
}

void TickSelfOnlineExtension(GTA_Native_Manager& natives) noexcept
{
    TickSkipCutscene(natives);
    TickOffTheRadar(natives);
}
}
