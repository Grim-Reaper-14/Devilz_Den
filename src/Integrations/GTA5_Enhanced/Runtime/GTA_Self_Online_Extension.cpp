#include "GTA_Self_Online_Extension.hpp"

#include "GTA_Gameplay_State.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Manager.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Registry.hpp"

#include <Windows.h>
#include <Psapi.h>

#include <algorithm>
#include <array>
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

constexpr std::array<int, 17> ScriptGlobalsPattern{
    0x48, 0x8B, 0x8E, 0xB8, 0x00, 0x00, 0x00, 0x48, 0x8D, 0x15,
    -1, -1, -1, -1, 0x49, 0x89, 0xD8
};
constexpr std::array<int, 19> ScriptThreadsPattern{
    0x48, 0x8B, 0x05, -1, -1, -1, -1, 0x48, 0x89, 0x34,
    0xF8, 0x48, 0xFF, 0xC7, 0x48, 0x39, 0xFB, 0x75, 0x97
};

struct SelfOnlineRuntime
{
    std::uintptr_t scriptGlobalsAddress = 0;
    std::uintptr_t scriptThreadsStorageAddress = 0;
    bool resolutionAttempted = false;
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

bool ReadableProtection(DWORD protection) noexcept
{
    protection &= 0xFFU;
    return protection == PAGE_READONLY ||
           protection == PAGE_READWRITE ||
           protection == PAGE_WRITECOPY ||
           protection == PAGE_EXECUTE_READ ||
           protection == PAGE_EXECUTE_READWRITE ||
           protection == PAGE_EXECUTE_WRITECOPY;
}

bool IsReadableAddress(std::uintptr_t address, std::size_t size) noexcept
{
    if (address == 0 || size == 0)
        return false;

    MEMORY_BASIC_INFORMATION memory{};
    if (::VirtualQuery(reinterpret_cast<const void*>(address), &memory, sizeof(memory)) == 0)
        return false;

    if (memory.State != MEM_COMMIT || (memory.Protect & (PAGE_GUARD | PAGE_NOACCESS)) != 0 ||
        !ReadableProtection(memory.Protect)) {
        return false;
    }

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

template <std::size_t N>
std::uintptr_t FindPattern(std::uintptr_t base, std::size_t size, const std::array<int, N>& pattern) noexcept
{
    if (base == 0 || size < N || base > (std::numeric_limits<std::uintptr_t>::max)() - size)
        return 0;

    const auto imageEnd = base + size;
    std::uintptr_t cursor = base;
    while (cursor < imageEnd) {
        MEMORY_BASIC_INFORMATION memory{};
        if (::VirtualQuery(reinterpret_cast<const void*>(cursor), &memory, sizeof(memory)) == 0)
            break;

        const auto regionBase = reinterpret_cast<std::uintptr_t>(memory.BaseAddress);
        if (regionBase > (std::numeric_limits<std::uintptr_t>::max)() - memory.RegionSize)
            break;
        const auto regionEnd = (std::min)(imageEnd, regionBase + memory.RegionSize);
        const auto scanBegin = (std::max)(cursor, regionBase);

        const bool readable = memory.State == MEM_COMMIT &&
                              (memory.Protect & (PAGE_GUARD | PAGE_NOACCESS)) == 0 &&
                              ReadableProtection(memory.Protect);
        if (readable && regionEnd > scanBegin && regionEnd - scanBegin >= N) {
            const auto* bytes = reinterpret_cast<const std::uint8_t*>(scanBegin);
            const std::size_t regionSize = regionEnd - scanBegin;
            for (std::size_t offset = 0; offset + N <= regionSize; ++offset) {
                bool match = true;
                for (std::size_t index = 0; index < N; ++index) {
                    if (pattern[index] >= 0 &&
                        bytes[offset + index] != static_cast<std::uint8_t>(pattern[index])) {
                        match = false;
                        break;
                    }
                }
                if (match)
                    return scanBegin + offset;
            }
        }

        if (regionEnd <= cursor)
            break;
        cursor = regionEnd;
    }

    return 0;
}

std::uintptr_t ResolveRipRelative32(std::uintptr_t displacementAddress) noexcept
{
    if (!IsReadableAddress(displacementAddress, sizeof(std::int32_t)))
        return 0;

    std::int32_t displacement = 0;
    std::memcpy(&displacement, reinterpret_cast<const void*>(displacementAddress), sizeof(displacement));
    return static_cast<std::uintptr_t>(
        static_cast<std::intptr_t>(displacementAddress + sizeof(displacement)) + displacement);
}

bool EnsureRuntimeTargets() noexcept
{
    if (g_runtime.scriptGlobalsAddress != 0 && g_runtime.scriptThreadsStorageAddress != 0)
        return true;
    if (g_runtime.resolutionAttempted)
        return false;

    g_runtime.resolutionAttempted = true;
    const HMODULE module = ::GetModuleHandleW(L"GTA5_Enhanced.exe");
    if (!module)
        return false;

    MODULEINFO info{};
    if (::GetModuleInformation(::GetCurrentProcess(), module, &info, sizeof(info)) == FALSE)
        return false;

    const auto base = reinterpret_cast<std::uintptr_t>(info.lpBaseOfDll);
    const auto imageSize = static_cast<std::size_t>(info.SizeOfImage);

    const auto globalsMatch = FindPattern(base, imageSize, ScriptGlobalsPattern);
    const auto threadsMatch = FindPattern(base, imageSize, ScriptThreadsPattern);
    if (globalsMatch == 0 || threadsMatch == 0)
        return false;

    // Matches the same Enhanced resolve chains used by the project's target registry.
    g_runtime.scriptGlobalsAddress = ResolveRipRelative32(globalsMatch + 10U);
    g_runtime.scriptThreadsStorageAddress = ResolveRipRelative32(threadsMatch + 3U);
    return g_runtime.scriptGlobalsAddress != 0 && g_runtime.scriptThreadsStorageAddress != 0;
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
    if (!EnsureRuntimeTargets() ||
        !IsReadableAddress(g_runtime.scriptThreadsStorageAddress, sizeof(GTA_Script_Thread_Array_View))) {
        return nullptr;
    }

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
    if (!EnsureRuntimeTargets())
        return nullptr;

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
    if (!player || *player < 0 || *player >= 32 || !SafeToModifyFreemodeGlobals(*player))
        return;

    auto* active = ResolveGlobal<int>(PlayerEntryGlobal(*player, GlobalPlayerOffRadarOffset));
    if (!active)
        return;

    if (!desired) {
        *active = 0;
        g_offRadarApplied = false;
        return;
    }

    const auto networkTime = natives.InvokeHash<int>(GetNetworkTimeHash);
    auto* timer = ResolveGlobal<int>(OffRadarTimerGlobal);
    if (!networkTime || !timer)
        return;

    *timer = *networkTime;
    *active = 1;
    g_offRadarApplied = true;
}

void TickSkipCutscene(GTA_Native_Manager& natives) noexcept
{
    if (!GTA_Gameplay_State::Instance().ConsumeSkipCutsceneRequest())
        return;

    (void)natives.InvokeHash<void>(StopCutsceneImmediatelyHash);
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
