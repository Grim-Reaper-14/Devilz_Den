#include "GTA_Bunker_Extension.hpp"

#include <Windows.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>

namespace Devilz::Integrations::GTA5_Enhanced
{
namespace
{
constexpr std::uint64_t SupportedFingerprint = 0x6A4F97F605B81000ULL;
constexpr std::uint32_t BunkerSellLocalBase = 1275U;
constexpr std::uint32_t BunkerSellCompletionOffset = 774U;
constexpr std::uint32_t BunkerSellCompletionLocal =
    BunkerSellLocalBase + BunkerSellCompletionOffset;
static_assert(BunkerSellCompletionLocal == 2049U);

constexpr std::uint32_t Joaat(std::string_view value) noexcept
{
    std::uint32_t hash = 0;
    for (const char rawCharacter : value) {
        auto character = static_cast<unsigned char>(rawCharacter);
        if (character >= static_cast<unsigned char>('A') &&
            character <= static_cast<unsigned char>('Z')) {
            character = static_cast<unsigned char>(character + ('a' - 'A'));
        }

        hash += character;
        hash += hash << 10U;
        hash ^= hash >> 6U;
    }

    hash += hash << 3U;
    hash ^= hash >> 11U;
    hash += hash << 15U;
    return hash;
}

constexpr std::uint32_t GunrunningScriptHash = Joaat("gb_gunrunning");
static_assert(GunrunningScriptHash == 0x8CD0BFF0U);

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

static_assert(offsetof(GTA_Script_Thread_View, stack) == 0xB8);
static_assert(offsetof(GTA_Script_Thread_View, scriptHash) == 0x150);

struct GTA_Script_Thread_Array_View
{
    GTA_Script_Thread_View** data = nullptr;
    std::uint16_t size = 0;
    std::uint16_t capacity = 0;
    std::uint32_t pad0C = 0;
};

static_assert(sizeof(GTA_Script_Thread_Array_View) == 0x10);

struct BunkerRuntime
{
    std::uintptr_t scriptThreadsStorageAddress = 0;
    std::uint64_t buildFingerprint = 0;
};

struct BunkerActionResult
{
    GTA_Bunker_Instant_Sell_Status status = GTA_Bunker_Instant_Sell_Status::Failed;
    std::string detail;
};

BunkerRuntime g_runtime{};
std::mutex g_stateMutex;
GTA_Bunker_Instant_Sell_Snapshot g_snapshot{};
bool g_requestPending = false;
std::uint64_t g_nextRequestId = 1;

bool IsReadableAddress(std::uintptr_t address, std::size_t size) noexcept
{
    if (address == 0 || size == 0)
        return false;

    MEMORY_BASIC_INFORMATION memory{};
    if (::VirtualQuery(reinterpret_cast<const void*>(address), &memory, sizeof(memory)) == 0)
        return false;

    if (memory.State != MEM_COMMIT ||
        (memory.Protect & (PAGE_GUARD | PAGE_NOACCESS)) != 0) {
        return false;
    }

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
    return protection == PAGE_READWRITE ||
           protection == PAGE_WRITECOPY ||
           protection == PAGE_EXECUTE_READWRITE ||
           protection == PAGE_EXECUTE_WRITECOPY;
}

GTA_Script_Thread_View* FindGunrunningThread() noexcept
{
    if (!IsReadableAddress(
            g_runtime.scriptThreadsStorageAddress,
            sizeof(GTA_Script_Thread_Array_View))) {
        return nullptr;
    }

    GTA_Script_Thread_Array_View threads{};
    std::memcpy(
        &threads,
        reinterpret_cast<const void*>(g_runtime.scriptThreadsStorageAddress),
        sizeof(threads));

    const auto count = (std::min<std::size_t>)(threads.size, 256U);
    if (!threads.data || count == 0 ||
        !IsReadableAddress(
            reinterpret_cast<std::uintptr_t>(threads.data),
            count * sizeof(void*))) {
        return nullptr;
    }

    for (std::size_t index = 0; index < count; ++index) {
        GTA_Script_Thread_View* thread = nullptr;
        std::memcpy(&thread, threads.data + index, sizeof(thread));
        if (!thread ||
            !IsReadableAddress(
                reinterpret_cast<std::uintptr_t>(thread),
                sizeof(GTA_Script_Thread_View))) {
            continue;
        }

        if (thread->context.threadId != 0 &&
            thread->scriptHash == GunrunningScriptHash) {
            return thread;
        }
    }

    return nullptr;
}

BunkerActionResult CompleteBunkerSellMission()
{
    if (g_runtime.buildFingerprint != SupportedFingerprint) {
        return {
            GTA_Bunker_Instant_Sell_Status::UnsupportedBuild,
            "Bunker Instant-Sell is only registered for Enhanced 1.73 / b1158.13."};
    }

    if (g_runtime.scriptThreadsStorageAddress == 0) {
        return {
            GTA_Bunker_Instant_Sell_Status::RuntimeUnavailable,
            "ScriptThreads is unavailable; gb_gunrunning cannot be resolved."};
    }

    auto* thread = FindGunrunningThread();
    if (!thread) {
        return {
            GTA_Bunker_Instant_Sell_Status::ScriptNotRunning,
            "gb_gunrunning is not active. Start a Bunker sell mission first."};
    }

    if (!thread->stack || thread->context.stackSize <= BunkerSellCompletionLocal) {
        return {
            GTA_Bunker_Instant_Sell_Status::LocalUnavailable,
            "gb_gunrunning stack is smaller than the registered local layout."};
    }

    const auto localAddress = reinterpret_cast<std::uintptr_t>(
        thread->stack + BunkerSellCompletionLocal);
    if (!IsReadableAddress(localAddress, sizeof(std::int32_t)) ||
        !IsWritableAddress(localAddress, sizeof(std::int32_t))) {
        return {
            GTA_Bunker_Instant_Sell_Status::LocalUnavailable,
            "gb_gunrunning local 2049 is not safely writable."};
    }

    constexpr std::int32_t CompleteMission = 0;
    std::memcpy(
        reinterpret_cast<void*>(localAddress),
        &CompleteMission,
        sizeof(CompleteMission));

    std::int32_t readback = -1;
    std::memcpy(
        &readback,
        reinterpret_cast<const void*>(localAddress),
        sizeof(readback));

    if (readback != CompleteMission) {
        return {
            GTA_Bunker_Instant_Sell_Status::Failed,
            "Bunker Instant-Sell failed readback verification."};
    }

    return {
        GTA_Bunker_Instant_Sell_Status::Succeeded,
        "gb_gunrunning local 2049 was set to 0 and verified."};
}
}

void ConfigureBunkerExtension(
    std::uintptr_t scriptThreadsStorageAddress,
    std::uint64_t buildFingerprint) noexcept
{
    std::scoped_lock lock(g_stateMutex);
    g_runtime.scriptThreadsStorageAddress = scriptThreadsStorageAddress;
    g_runtime.buildFingerprint = buildFingerprint;
    g_snapshot = {};
    g_requestPending = false;
    g_nextRequestId = 1;
}

void ResetBunkerExtension() noexcept
{
    std::scoped_lock lock(g_stateMutex);
    g_runtime = {};
    g_snapshot = {};
    g_requestPending = false;
    g_nextRequestId = 1;
}

bool RequestBunkerInstantSell() noexcept
{
    std::scoped_lock lock(g_stateMutex);
    if (g_requestPending ||
        g_snapshot.status == GTA_Bunker_Instant_Sell_Status::Queued) {
        return false;
    }

    g_requestPending = true;
    ++g_snapshot.revision;
    g_snapshot.requestId = g_nextRequestId++;
    g_snapshot.status = GTA_Bunker_Instant_Sell_Status::Queued;
    g_snapshot.detail = "Queued for the GTA game thread.";
    return true;
}

GTA_Bunker_Instant_Sell_Snapshot BunkerInstantSellSnapshot()
{
    std::scoped_lock lock(g_stateMutex);
    return g_snapshot;
}

const char* GTA_Bunker_Instant_Sell_Status_Name(
    GTA_Bunker_Instant_Sell_Status status) noexcept
{
    switch (status) {
    case GTA_Bunker_Instant_Sell_Status::Idle: return "IDLE";
    case GTA_Bunker_Instant_Sell_Status::Queued: return "QUEUED";
    case GTA_Bunker_Instant_Sell_Status::Succeeded: return "SUCCEEDED";
    case GTA_Bunker_Instant_Sell_Status::RuntimeUnavailable: return "RUNTIME UNAVAILABLE";
    case GTA_Bunker_Instant_Sell_Status::UnsupportedBuild: return "UNSUPPORTED BUILD";
    case GTA_Bunker_Instant_Sell_Status::ScriptNotRunning: return "SCRIPT NOT RUNNING";
    case GTA_Bunker_Instant_Sell_Status::LocalUnavailable: return "LOCAL UNAVAILABLE";
    case GTA_Bunker_Instant_Sell_Status::Failed: return "FAILED";
    default: return "UNKNOWN";
    }
}

void TickBunkerExtension() noexcept
{
    {
        std::scoped_lock lock(g_stateMutex);
        if (!g_requestPending)
            return;
        g_requestPending = false;
    }

    auto result = CompleteBunkerSellMission();

    std::scoped_lock lock(g_stateMutex);
    ++g_snapshot.revision;
    g_snapshot.status = result.status;
    g_snapshot.detail = std::move(result.detail);
}
}
