#include "GTA_Casino_Extension.hpp"

#include "Backend/Logging/LoggerService.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Manager.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Registry.hpp"
#include "Integrations/GTA5_Enhanced/Script/Globals/Script_Global_Manager.hpp"

#include <Windows.h>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <mutex>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace Devilz::Integrations::GTA5_Enhanced
{
namespace
{
constexpr int MinimumSpinCount = 1;
constexpr int MaximumSpinCount = 4;

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

struct CasinoRuntime
{
    Script_Global_Manager* globals = nullptr;
    std::uintptr_t scriptThreadsStorageAddress = 0;
    std::uint64_t buildFingerprint = 0;
};

CasinoRuntime g_runtime{};
std::atomic_int g_requestedMaxSpins{1};
std::atomic_bool g_requestedAdditionalSpins{true};
std::atomic_int g_requestedGtaPlusMaxSpins{2};
std::atomic_int g_selectedOutcome{18};
std::atomic_bool g_forceOutcome{false};

std::mutex g_stateMutex;
GTA_Casino_Snapshot g_snapshot{};
bool g_settingsRequestPending = false;

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

constexpr std::uint32_t LuckyWheelScriptHash = Joaat("casino_lucky_wheel");
static_assert(LuckyWheelScriptHash == 0xBDF0CCFFU);

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

GTA_Script_Thread_View* FindLuckyWheelThread() noexcept
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

        if (thread->context.threadId != 0 && thread->scriptHash == LuckyWheelScriptHash)
            return thread;
    }

    return nullptr;
}

template <typename T>
bool ReadGlobal(Script_Global_Manager& globals, std::uint32_t index, T& value)
{
    auto result = globals.Get(index).Read<T>();
    if (!result)
        return false;

    value = result.Value();
    return true;
}

template <typename T>
bool WriteGlobalVerified(
    Script_Global_Manager& globals,
    std::uint32_t index,
    const T& value)
{
    static_assert(std::is_trivially_copyable_v<T>);

    auto pointer = globals.Get(index).Resolve();
    if (!pointer)
        return false;

    std::memcpy(
        reinterpret_cast<void*>(pointer.Value().Address()),
        &value,
        sizeof(value));

    auto readback = globals.Get(index).Read<T>();
    if (!readback)
        return false;

    const auto actual = readback.Value();
    return std::memcmp(&actual, &value, sizeof(T)) == 0;
}

struct SettingsResult
{
    GTA_Lucky_Wheel_Settings_Status status = GTA_Lucky_Wheel_Settings_Status::Failed;
    std::string detail;
};

SettingsResult ApplyLuckyWheelSpinSettings()
{
    auto* globals = g_runtime.globals;
    if (!globals || !globals->Ready()) {
        return {
            GTA_Lucky_Wheel_Settings_Status::RuntimeUnavailable,
            "Script globals are not configured."};
    }

    if (g_runtime.buildFingerprint != GTA_Casino_Supported_Fingerprint) {
        return {
            GTA_Lucky_Wheel_Settings_Status::UnsupportedBuild,
            "Lucky Wheel globals are not registered for this GTA build."};
    }

    int oldMaxSpins = 0;
    int oldAdditionalSpins = 0;
    int oldGtaPlusMaxSpins = 0;
    if (!ReadGlobal(*globals, GTA_Lucky_Wheel_Max_Spins_Global, oldMaxSpins) ||
        !ReadGlobal(*globals, GTA_Lucky_Wheel_Additional_Spins_Global, oldAdditionalSpins) ||
        !ReadGlobal(*globals, GTA_Lucky_Wheel_Gta_Plus_Max_Spins_Global, oldGtaPlusMaxSpins)) {
        return {
            GTA_Lucky_Wheel_Settings_Status::Failed,
            "The current Lucky Wheel tunables could not be read safely."};
    }

    const int maxSpins = std::clamp(
        g_requestedMaxSpins.load(std::memory_order_acquire),
        MinimumSpinCount,
        MaximumSpinCount);
    const int additionalSpins =
        g_requestedAdditionalSpins.load(std::memory_order_acquire) ? 1 : 0;
    const int gtaPlusMaxSpins = std::clamp(
        g_requestedGtaPlusMaxSpins.load(std::memory_order_acquire),
        MinimumSpinCount,
        MaximumSpinCount);

    const bool maxWritten = WriteGlobalVerified(
        *globals,
        GTA_Lucky_Wheel_Max_Spins_Global,
        maxSpins);
    const bool additionalWritten = WriteGlobalVerified(
        *globals,
        GTA_Lucky_Wheel_Additional_Spins_Global,
        additionalSpins);
    const bool gtaPlusWritten = WriteGlobalVerified(
        *globals,
        GTA_Lucky_Wheel_Gta_Plus_Max_Spins_Global,
        gtaPlusMaxSpins);

    if (!maxWritten || !additionalWritten || !gtaPlusWritten) {
        const bool maxRestored = WriteGlobalVerified(
            *globals,
            GTA_Lucky_Wheel_Max_Spins_Global,
            oldMaxSpins);
        const bool additionalRestored = WriteGlobalVerified(
            *globals,
            GTA_Lucky_Wheel_Additional_Spins_Global,
            oldAdditionalSpins);
        const bool gtaPlusRestored = WriteGlobalVerified(
            *globals,
            GTA_Lucky_Wheel_Gta_Plus_Max_Spins_Global,
            oldGtaPlusMaxSpins);
        const bool restored =
            maxRestored && additionalRestored && gtaPlusRestored;

        return {
            GTA_Lucky_Wheel_Settings_Status::Failed,
            restored
                ? "A Lucky Wheel tunable failed verification; the previous values were restored."
                : "A Lucky Wheel tunable failed verification and rollback was incomplete."};
    }

    return {
        GTA_Lucky_Wheel_Settings_Status::Succeeded,
        "Lucky Wheel spin limits applied and verified."};
}

void CompleteSettingsRequest(SettingsResult result)
{
    std::scoped_lock lock(g_stateMutex);
    ++g_snapshot.revision;
    g_snapshot.settingsStatus = result.status;
    g_snapshot.settingsDetail = std::move(result.detail);
}

void RefreshLiveSettings(GTA_Casino_Snapshot& next)
{
    auto* globals = g_runtime.globals;
    next.liveSettingsAvailable = false;
    if (!next.buildSupported || !globals || !globals->Ready())
        return;

    int maxSpins = 0;
    int additionalSpins = 0;
    int gtaPlusMaxSpins = 0;
    if (!ReadGlobal(*globals, GTA_Lucky_Wheel_Max_Spins_Global, maxSpins) ||
        !ReadGlobal(*globals, GTA_Lucky_Wheel_Additional_Spins_Global, additionalSpins) ||
        !ReadGlobal(*globals, GTA_Lucky_Wheel_Gta_Plus_Max_Spins_Global, gtaPlusMaxSpins)) {
        return;
    }

    next.liveSettingsAvailable = true;
    next.liveMaxSpins = maxSpins;
    next.liveAdditionalSpins = additionalSpins != 0;
    next.liveGtaPlusMaxSpins = gtaPlusMaxSpins;
}

void RefreshLuckyWheelLocal(GTA_Native_Manager& natives, GTA_Casino_Snapshot& next)
{
    next.luckyWheelScriptActive = false;
    next.outcomeLocalReady = false;
    next.currentOutcome = -1;
    next.currentOutcomeLocal = 0;

    if (!next.buildSupported || !next.scriptThreadsReady) {
        next.runtimeDetail = next.buildSupported
            ? "ScriptThreads is unavailable; the Lucky Wheel local cannot be resolved."
            : "Lucky Wheel offsets are not verified for this GTA build.";
        return;
    }

    auto* thread = FindLuckyWheelThread();
    if (!thread) {
        next.runtimeDetail =
            "casino_lucky_wheel is not active. Enter the Diamond Casino to load it.";
        return;
    }

    next.luckyWheelScriptActive = true;
    const auto player = natives.Invoke<int>(GTA_Native_Id::PlayerId);
    if (!player || *player < 0 || *player >= 32) {
        next.runtimeDetail = "The local GTA Online player index is unavailable.";
        return;
    }

    const auto localIndex = GTA_Lucky_Wheel_Outcome_Local_Index(
        static_cast<std::uint32_t>(*player));
    next.currentOutcomeLocal = localIndex;
    if (!thread->stack || thread->context.stackSize <= localIndex) {
        next.runtimeDetail = "The Lucky Wheel script stack is smaller than the verified local layout.";
        return;
    }

    const auto localAddress = reinterpret_cast<std::uintptr_t>(thread->stack + localIndex);
    if (!IsReadableAddress(localAddress, sizeof(std::int32_t))) {
        next.runtimeDetail = "The Lucky Wheel outcome local is not readable.";
        return;
    }

    next.outcomeLocalReady = true;
    std::int32_t currentOutcome = -1;
    std::memcpy(
        &currentOutcome,
        reinterpret_cast<const void*>(localAddress),
        sizeof(currentOutcome));
    next.currentOutcome = currentOutcome;

    if (!next.forceOutcome) {
        next.runtimeDetail = currentOutcome >= 0 &&
                currentOutcome < GTA_Lucky_Wheel_Outcome_Count
            ? std::string{"Wheel script active; current outcome is "} +
                GTA_Lucky_Wheel_Outcome_Name(currentOutcome) + "."
            : "Wheel script active; no spin outcome is currently assigned.";
        return;
    }

    if (!IsWritableAddress(localAddress, sizeof(std::int32_t))) {
        next.runtimeDetail = "The Lucky Wheel outcome local is not writable.";
        return;
    }

    const std::int32_t selectedOutcome = static_cast<std::int32_t>(std::clamp(
        next.selectedOutcome,
        0,
        GTA_Lucky_Wheel_Outcome_Count - 1));
    std::memcpy(
        reinterpret_cast<void*>(localAddress),
        &selectedOutcome,
        sizeof(selectedOutcome));

    std::int32_t readback = -1;
    std::memcpy(
        &readback,
        reinterpret_cast<const void*>(localAddress),
        sizeof(readback));
    next.currentOutcome = readback;
    if (readback != selectedOutcome) {
        next.runtimeDetail = "The Lucky Wheel outcome override failed readback verification.";
        return;
    }

    next.runtimeDetail = std::string{"Forcing "} +
        GTA_Lucky_Wheel_Outcome_Name(selectedOutcome) + " at local " +
        std::to_string(localIndex) + ".";
}
}

void ConfigureCasinoExtension(
    Script_Global_Manager* globals,
    std::uintptr_t scriptThreadsStorageAddress,
    std::uint64_t buildFingerprint,
    Backend::LoggerService* logger) noexcept
{
    ResetCasinoExtension();
    g_runtime.globals = globals;
    g_runtime.scriptThreadsStorageAddress = scriptThreadsStorageAddress;
    g_runtime.buildFingerprint = buildFingerprint;

    {
        std::scoped_lock lock(g_stateMutex);
        g_snapshot.buildSupported =
            buildFingerprint == GTA_Casino_Supported_Fingerprint;
        g_snapshot.globalsReady = globals && globals->Ready();
        g_snapshot.scriptThreadsReady = scriptThreadsStorageAddress != 0;
        g_snapshot.runtimeDetail = g_snapshot.buildSupported
            ? "Waiting for the Lucky Wheel script."
            : "Lucky Wheel globals and locals are not verified for this GTA build.";
    }

    if (logger) {
        const bool ready =
            buildFingerprint == GTA_Casino_Supported_Fingerprint &&
            scriptThreadsStorageAddress != 0;
        logger->Log(
            ready ? Backend::LogLevel::Info : Backend::LogLevel::Warning,
            ready
                ? "Casino Lucky Wheel ready | Enhanced globals and player-stride local registered"
                : "Casino Lucky Wheel unavailable | build fingerprint or ScriptThreads is not verified",
            "GTA5_Enhanced.Casino");
    }
}

void ResetCasinoExtension() noexcept
{
    g_runtime = {};
    g_requestedMaxSpins.store(1, std::memory_order_release);
    g_requestedAdditionalSpins.store(true, std::memory_order_release);
    g_requestedGtaPlusMaxSpins.store(2, std::memory_order_release);
    g_selectedOutcome.store(18, std::memory_order_release);
    g_forceOutcome.store(false, std::memory_order_release);

    std::scoped_lock lock(g_stateMutex);
    g_snapshot = {};
    g_settingsRequestPending = false;
}

void TickCasinoExtension(GTA_Native_Manager& natives) noexcept
{
    bool applySettings = false;
    {
        std::scoped_lock lock(g_stateMutex);
        applySettings = g_settingsRequestPending;
        g_settingsRequestPending = false;
    }

    if (applySettings)
        CompleteSettingsRequest(ApplyLuckyWheelSpinSettings());

    GTA_Casino_Snapshot next{};
    {
        std::scoped_lock lock(g_stateMutex);
        next = g_snapshot;
    }

    ++next.revision;
    next.buildSupported =
        g_runtime.buildFingerprint == GTA_Casino_Supported_Fingerprint;
    next.globalsReady = g_runtime.globals && g_runtime.globals->Ready();
    next.scriptThreadsReady = g_runtime.scriptThreadsStorageAddress != 0;
    next.requestedMaxSpins =
        g_requestedMaxSpins.load(std::memory_order_acquire);
    next.requestedAdditionalSpins =
        g_requestedAdditionalSpins.load(std::memory_order_acquire);
    next.requestedGtaPlusMaxSpins =
        g_requestedGtaPlusMaxSpins.load(std::memory_order_acquire);
    next.selectedOutcome = g_selectedOutcome.load(std::memory_order_acquire);
    next.forceOutcome = g_forceOutcome.load(std::memory_order_acquire);

    RefreshLiveSettings(next);
    RefreshLuckyWheelLocal(natives, next);

    std::scoped_lock lock(g_stateMutex);
    g_snapshot = std::move(next);
}

void SetLuckyWheelSpinSettings(
    int maxSpins,
    bool additionalSpins,
    int gtaPlusMaxSpins) noexcept
{
    maxSpins = std::clamp(maxSpins, MinimumSpinCount, MaximumSpinCount);
    gtaPlusMaxSpins = std::clamp(
        gtaPlusMaxSpins,
        MinimumSpinCount,
        MaximumSpinCount);
    g_requestedMaxSpins.store(maxSpins, std::memory_order_release);
    g_requestedAdditionalSpins.store(additionalSpins, std::memory_order_release);
    g_requestedGtaPlusMaxSpins.store(gtaPlusMaxSpins, std::memory_order_release);

    std::scoped_lock lock(g_stateMutex);
    ++g_snapshot.revision;
    g_snapshot.requestedMaxSpins = maxSpins;
    g_snapshot.requestedAdditionalSpins = additionalSpins;
    g_snapshot.requestedGtaPlusMaxSpins = gtaPlusMaxSpins;
}

bool RequestApplyLuckyWheelSpinSettings()
{
    std::scoped_lock lock(g_stateMutex);
    if (g_settingsRequestPending ||
        g_snapshot.settingsStatus == GTA_Lucky_Wheel_Settings_Status::Queued) {
        return false;
    }

    g_settingsRequestPending = true;
    ++g_snapshot.revision;
    g_snapshot.settingsStatus = GTA_Lucky_Wheel_Settings_Status::Queued;
    g_snapshot.settingsDetail = "Queued for the GTA game thread.";
    return true;
}

void SetLuckyWheelSelectedOutcome(int outcome) noexcept
{
    outcome = std::clamp(outcome, 0, GTA_Lucky_Wheel_Outcome_Count - 1);
    g_selectedOutcome.store(outcome, std::memory_order_release);

    std::scoped_lock lock(g_stateMutex);
    ++g_snapshot.revision;
    g_snapshot.selectedOutcome = outcome;
}

void SetForceLuckyWheelOutcome(bool enabled) noexcept
{
    g_forceOutcome.store(enabled, std::memory_order_release);

    std::scoped_lock lock(g_stateMutex);
    ++g_snapshot.revision;
    g_snapshot.forceOutcome = enabled;
}

GTA_Casino_Snapshot CasinoSnapshot()
{
    std::scoped_lock lock(g_stateMutex);
    return g_snapshot;
}

const char* GTA_Lucky_Wheel_Settings_Status_Name(
    GTA_Lucky_Wheel_Settings_Status status) noexcept
{
    switch (status) {
    case GTA_Lucky_Wheel_Settings_Status::Idle: return "IDLE";
    case GTA_Lucky_Wheel_Settings_Status::Queued: return "QUEUED";
    case GTA_Lucky_Wheel_Settings_Status::Succeeded: return "SUCCEEDED";
    case GTA_Lucky_Wheel_Settings_Status::RuntimeUnavailable: return "RUNTIME UNAVAILABLE";
    case GTA_Lucky_Wheel_Settings_Status::UnsupportedBuild: return "UNSUPPORTED BUILD";
    case GTA_Lucky_Wheel_Settings_Status::Failed: return "FAILED";
    default: return "UNKNOWN";
    }
}
}
