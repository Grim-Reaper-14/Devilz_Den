// GTA Online Enhanced 1.73 Bunker business mod runtime.
#include "GTA_Bunker_Extension.hpp"

#include "GTA_Business_State.hpp"
#include "Integrations/GTA5_Enhanced/Script/Globals/Script_Global_Manager.hpp"

#include <Windows.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace Devilz::Integrations::GTA5_Enhanced
{
namespace
{
constexpr std::uint64_t SupportedFingerprint = 0x6A4F97F605B81000ULL;

// Enhanced 1.73 / b1158.13 Bunker tunables.
constexpr std::uint32_t TunablesBase = 262145U;
constexpr std::uint32_t BunkerHighDemandBonusOffset = 21232U;
constexpr std::uint32_t BunkerHighDemandMaxBonusOffset = 21233U;
constexpr std::uint32_t BunkerNearSaleMultiplierOffset = 21319U;
constexpr std::uint32_t BunkerFarSaleMultiplierOffset = 21320U;
constexpr std::uint32_t BunkerProductValueOffset = 21347U;
constexpr auto BunkerTunablesRefreshInterval = std::chrono::milliseconds(500);

// Enhanced 1.73 / b1158.13 Ammu-Nation Contract (Excess Weapon Parts).
constexpr std::uint32_t AmmuNationDeliveryPayoutOffset = 32173U;
constexpr std::uint32_t AmmuNationAmbushChanceOffset = 32175U;
constexpr std::uint32_t AmmuNationTimeLimitOffset = 32178U;
constexpr std::uint32_t PlayerBusinessDataBase = 2658296U;
constexpr std::uint32_t PlayerBusinessDataStride = 468U;
constexpr std::uint32_t ExcessWeaponPartsTriggerOffset = 465U;
constexpr int ExcessWeaponPartsTriggerBit = 1;
constexpr int ExcessWeaponPartsTriggerMask = 1 << ExcessWeaponPartsTriggerBit;
constexpr auto AmmuNationRefreshInterval = std::chrono::milliseconds(500);

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
    Script_Global_Manager* globals = nullptr;
    std::uintptr_t scriptThreadsStorageAddress = 0;
    std::uint64_t buildFingerprint = 0;
    std::chrono::steady_clock::time_point lastTunablesRefresh{};
    std::chrono::steady_clock::time_point lastAmmuNationRefresh{};
};

struct BunkerActionResult
{
    GTA_Bunker_Instant_Sell_Status status = GTA_Bunker_Instant_Sell_Status::Failed;
    std::string detail;
};

struct BunkerTunablesCommand
{
    std::uint64_t id = 0;
    int productValue = 0;
    float nearSaleMultiplier = 0.0F;
    float farSaleMultiplier = 0.0F;
    float highDemandBonus = 0.0F;
    float highDemandMaxBonus = 0.0F;
};

struct BunkerTunablesResult
{
    GTA_Bunker_Tunables_Status status = GTA_Bunker_Tunables_Status::Failed;
    std::string detail;
};

enum class BunkerAmmuNationCommandType : std::uint8_t
{
    ApplyTunables,
    TriggerExcessWeaponParts
};

struct BunkerAmmuNationCommand
{
    std::uint64_t id = 0;
    BunkerAmmuNationCommandType type = BunkerAmmuNationCommandType::ApplyTunables;
    int deliveryPayout = 0;
    int ambushChance = 0;
    int timeLimit = 0;
};

struct BunkerAmmuNationResult
{
    GTA_Bunker_Ammu_Nation_Status status = GTA_Bunker_Ammu_Nation_Status::Failed;
    std::string detail;
};

BunkerRuntime g_runtime{};
std::mutex g_stateMutex;
GTA_Bunker_Instant_Sell_Snapshot g_snapshot{};
bool g_requestPending = false;
std::uint64_t g_nextRequestId = 1;
GTA_Bunker_Tunables_Snapshot g_tunablesSnapshot{};
std::optional<BunkerTunablesCommand> g_pendingTunables;
GTA_Bunker_Tunables_Action_Snapshot g_tunablesAction{};
std::uint64_t g_nextTunablesRequestId = 1;
GTA_Bunker_Ammu_Nation_Snapshot g_ammuNationSnapshot{};
std::optional<BunkerAmmuNationCommand> g_pendingAmmuNation;
GTA_Bunker_Ammu_Nation_Action_Snapshot g_ammuNationAction{};
std::uint64_t g_nextAmmuNationRequestId = 1;

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

std::uint32_t PlayerExcessWeaponPartsEntry(int player) noexcept
{
    return PlayerBusinessDataBase +
        1U +
        static_cast<std::uint32_t>(player) * PlayerBusinessDataStride +
        ExcessWeaponPartsTriggerOffset;
}

int PublishedPlayerIndex() noexcept
{
    return GTA_Business_State::Instance().Nightclub().playerIndex;
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

GTA_Bunker_Tunables_Snapshot BuildBunkerTunablesSnapshot()
{
    GTA_Bunker_Tunables_Snapshot snapshot{};
    auto* globals = g_runtime.globals;

    if (!globals || !globals->Ready()) {
        snapshot.detail = "Script globals are not configured for Bunker tunables.";
        return snapshot;
    }

    if (g_runtime.buildFingerprint != SupportedFingerprint) {
        snapshot.detail = "Bunker tunables are not registered for this GTA build.";
        return snapshot;
    }

    bool ok = true;
    ok &= ReadGlobal(
        *globals,
        TunablesBase + BunkerProductValueOffset,
        snapshot.productValue);
    ok &= ReadGlobal(
        *globals,
        TunablesBase + BunkerNearSaleMultiplierOffset,
        snapshot.nearSaleMultiplier);
    ok &= ReadGlobal(
        *globals,
        TunablesBase + BunkerFarSaleMultiplierOffset,
        snapshot.farSaleMultiplier);
    ok &= ReadGlobal(
        *globals,
        TunablesBase + BunkerHighDemandBonusOffset,
        snapshot.highDemandBonus);
    ok &= ReadGlobal(
        *globals,
        TunablesBase + BunkerHighDemandMaxBonusOffset,
        snapshot.highDemandMaxBonus);

    snapshot.runtimeReady = ok;
    snapshot.detail = ok
        ? "Bunker product, sale multiplier, and high-demand tunables are ready."
        : "One or more Bunker tunables could not be read.";
    return snapshot;
}

BunkerTunablesResult ApplyBunkerTunables(const BunkerTunablesCommand& command)
{
    auto* globals = g_runtime.globals;
    if (!globals || !globals->Ready()) {
        return {
            GTA_Bunker_Tunables_Status::RuntimeUnavailable,
            "Script globals are not configured for Bunker tunables."};
    }

    if (g_runtime.buildFingerprint != SupportedFingerprint) {
        return {
            GTA_Bunker_Tunables_Status::UnsupportedBuild,
            "Bunker tunables are only registered for Enhanced 1.73 / b1158.13."};
    }

    const bool valid = command.productValue >= 0 &&
        std::isfinite(command.nearSaleMultiplier) &&
        std::isfinite(command.farSaleMultiplier) &&
        std::isfinite(command.highDemandBonus) &&
        std::isfinite(command.highDemandMaxBonus) &&
        command.nearSaleMultiplier >= 0.0F &&
        command.farSaleMultiplier >= 0.0F &&
        command.highDemandBonus >= 0.0F &&
        command.highDemandMaxBonus >= 0.0F;
    if (!valid) {
        return {
            GTA_Bunker_Tunables_Status::InvalidValue,
            "Bunker tunable values must be finite and non-negative."};
    }

    if (!WriteGlobalVerified(
            *globals,
            TunablesBase + BunkerProductValueOffset,
            command.productValue) ||
        !WriteGlobalVerified(
            *globals,
            TunablesBase + BunkerNearSaleMultiplierOffset,
            command.nearSaleMultiplier) ||
        !WriteGlobalVerified(
            *globals,
            TunablesBase + BunkerFarSaleMultiplierOffset,
            command.farSaleMultiplier) ||
        !WriteGlobalVerified(
            *globals,
            TunablesBase + BunkerHighDemandBonusOffset,
            command.highDemandBonus) ||
        !WriteGlobalVerified(
            *globals,
            TunablesBase + BunkerHighDemandMaxBonusOffset,
            command.highDemandMaxBonus)) {
        return {
            GTA_Bunker_Tunables_Status::Failed,
            "One or more Bunker tunable writes failed readback verification."};
    }

    return {
        GTA_Bunker_Tunables_Status::Succeeded,
        "Bunker globals were applied and verified."};
}

GTA_Bunker_Ammu_Nation_Snapshot BuildBunkerAmmuNationSnapshot()
{
    GTA_Bunker_Ammu_Nation_Snapshot snapshot{};
    auto* globals = g_runtime.globals;

    if (!globals || !globals->Ready()) {
        snapshot.detail = "Script globals are not configured for the Ammu-Nation Contract.";
        return snapshot;
    }

    if (g_runtime.buildFingerprint != SupportedFingerprint) {
        snapshot.detail = "Ammu-Nation Contract globals are not registered for this GTA build.";
        return snapshot;
    }

    bool ok = true;
    ok &= ReadGlobal(
        *globals,
        TunablesBase + AmmuNationDeliveryPayoutOffset,
        snapshot.deliveryPayout);
    ok &= ReadGlobal(
        *globals,
        TunablesBase + AmmuNationAmbushChanceOffset,
        snapshot.ambushChance);
    ok &= ReadGlobal(
        *globals,
        TunablesBase + AmmuNationTimeLimitOffset,
        snapshot.timeLimit);

    snapshot.runtimeReady = ok;
    if (!ok) {
        snapshot.detail = "One or more Ammu-Nation Contract tunables could not be read.";
        return snapshot;
    }

    const int player = PublishedPlayerIndex();
    if (player < 0 || player >= 32) {
        snapshot.detail = "Ammu-Nation Contract tunables are ready; local player index is not published yet.";
        return snapshot;
    }

    snapshot.playerIndex = player;
    int triggerFlags = 0;
    if (!ReadGlobal(*globals, PlayerExcessWeaponPartsEntry(player), triggerFlags)) {
        snapshot.detail = "Ammu-Nation Contract tunables are ready; trigger flags could not be read.";
        return snapshot;
    }

    snapshot.triggerBitEnabled =
        (triggerFlags & ExcessWeaponPartsTriggerMask) != 0;
    snapshot.detail = "Ammu-Nation Contract tunables and player trigger flag are ready.";
    return snapshot;
}

BunkerAmmuNationResult ExecuteBunkerAmmuNationCommand(
    const BunkerAmmuNationCommand& command)
{
    auto* globals = g_runtime.globals;
    if (!globals || !globals->Ready()) {
        return {
            GTA_Bunker_Ammu_Nation_Status::RuntimeUnavailable,
            "Script globals are not configured for the Ammu-Nation Contract."};
    }

    if (g_runtime.buildFingerprint != SupportedFingerprint) {
        return {
            GTA_Bunker_Ammu_Nation_Status::UnsupportedBuild,
            "Ammu-Nation Contract controls are only registered for Enhanced 1.73 / b1158.13."};
    }

    if (command.type == BunkerAmmuNationCommandType::ApplyTunables) {
        if (command.deliveryPayout < 0 ||
            command.ambushChance < 0 ||
            command.ambushChance > 100 ||
            command.timeLimit < 0) {
            return {
                GTA_Bunker_Ammu_Nation_Status::InvalidValue,
                "Payout/time must be non-negative and ambush chance must be 0-100."};
        }

        if (!WriteGlobalVerified(
                *globals,
                TunablesBase + AmmuNationDeliveryPayoutOffset,
                command.deliveryPayout) ||
            !WriteGlobalVerified(
                *globals,
                TunablesBase + AmmuNationAmbushChanceOffset,
                command.ambushChance) ||
            !WriteGlobalVerified(
                *globals,
                TunablesBase + AmmuNationTimeLimitOffset,
                command.timeLimit)) {
            return {
                GTA_Bunker_Ammu_Nation_Status::Failed,
                "One or more Ammu-Nation Contract tunable writes failed readback verification."};
        }

        return {
            GTA_Bunker_Ammu_Nation_Status::Succeeded,
            "Ammu-Nation Contract tunables were applied and verified."};
    }

    const int player = PublishedPlayerIndex();
    if (player < 0 || player >= 32) {
        return {
            GTA_Bunker_Ammu_Nation_Status::PlayerUnavailable,
            "Local player index is unavailable; Excess Weapon Parts was not triggered."};
    }

    const auto triggerIndex = PlayerExcessWeaponPartsEntry(player);
    int triggerFlags = 0;
    if (!ReadGlobal(*globals, triggerIndex, triggerFlags)) {
        return {
            GTA_Bunker_Ammu_Nation_Status::Failed,
            "Excess Weapon Parts trigger flags could not be read."};
    }

    const int updatedFlags = triggerFlags | ExcessWeaponPartsTriggerMask;
    if (updatedFlags != triggerFlags &&
        !WriteGlobalVerified(*globals, triggerIndex, updatedFlags)) {
        return {
            GTA_Bunker_Ammu_Nation_Status::Failed,
            "Excess Weapon Parts trigger bit failed readback verification."};
    }

    return {
        GTA_Bunker_Ammu_Nation_Status::Succeeded,
        updatedFlags == triggerFlags
            ? "Excess Weapon Parts trigger bit 1 is already enabled."
            : "Excess Weapon Parts trigger bit 1 was enabled and verified."};
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

void ConfigureBunkerGlobals(
    Script_Global_Manager* globals,
    std::uint64_t buildFingerprint) noexcept
{
    std::scoped_lock lock(g_stateMutex);
    g_runtime.globals = globals;
    g_runtime.buildFingerprint = buildFingerprint;
    g_runtime.lastTunablesRefresh = {};
    g_runtime.lastAmmuNationRefresh = {};
    g_tunablesSnapshot = {};
    g_pendingTunables.reset();
    g_tunablesAction = {};
    g_nextTunablesRequestId = 1;
    g_ammuNationSnapshot = {};
    g_pendingAmmuNation.reset();
    g_ammuNationAction = {};
    g_nextAmmuNationRequestId = 1;
}

void ResetBunkerExtension() noexcept
{
    std::scoped_lock lock(g_stateMutex);
    g_runtime = {};
    g_snapshot = {};
    g_requestPending = false;
    g_nextRequestId = 1;
    g_tunablesSnapshot = {};
    g_pendingTunables.reset();
    g_tunablesAction = {};
    g_nextTunablesRequestId = 1;
    g_ammuNationSnapshot = {};
    g_pendingAmmuNation.reset();
    g_ammuNationAction = {};
    g_nextAmmuNationRequestId = 1;
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

bool RequestBunkerTunables(
    int productValue,
    float nearSaleMultiplier,
    float farSaleMultiplier,
    float highDemandBonus,
    float highDemandMaxBonus) noexcept
{
    std::scoped_lock lock(g_stateMutex);
    if (g_pendingTunables ||
        g_tunablesAction.status == GTA_Bunker_Tunables_Status::Queued) {
        return false;
    }

    BunkerTunablesCommand command{};
    command.id = g_nextTunablesRequestId++;
    command.productValue = productValue;
    command.nearSaleMultiplier = nearSaleMultiplier;
    command.farSaleMultiplier = farSaleMultiplier;
    command.highDemandBonus = highDemandBonus;
    command.highDemandMaxBonus = highDemandMaxBonus;
    g_pendingTunables = command;

    ++g_tunablesAction.revision;
    g_tunablesAction.requestId = command.id;
    g_tunablesAction.status = GTA_Bunker_Tunables_Status::Queued;
    g_tunablesAction.detail = "Queued for the GTA game thread.";
    return true;
}

GTA_Bunker_Tunables_Snapshot BunkerTunablesSnapshot()
{
    std::scoped_lock lock(g_stateMutex);
    return g_tunablesSnapshot;
}

GTA_Bunker_Tunables_Action_Snapshot BunkerTunablesActionSnapshot()
{
    std::scoped_lock lock(g_stateMutex);
    return g_tunablesAction;
}

const char* GTA_Bunker_Tunables_Status_Name(
    GTA_Bunker_Tunables_Status status) noexcept
{
    switch (status) {
    case GTA_Bunker_Tunables_Status::Idle: return "IDLE";
    case GTA_Bunker_Tunables_Status::Queued: return "QUEUED";
    case GTA_Bunker_Tunables_Status::Succeeded: return "SUCCEEDED";
    case GTA_Bunker_Tunables_Status::RuntimeUnavailable: return "RUNTIME UNAVAILABLE";
    case GTA_Bunker_Tunables_Status::UnsupportedBuild: return "UNSUPPORTED BUILD";
    case GTA_Bunker_Tunables_Status::InvalidValue: return "INVALID VALUE";
    case GTA_Bunker_Tunables_Status::Failed: return "FAILED";
    default: return "UNKNOWN";
    }
}

bool RequestBunkerAmmuNationTunables(
    int deliveryPayout,
    int ambushChance,
    int timeLimit) noexcept
{
    std::scoped_lock lock(g_stateMutex);
    if (g_pendingAmmuNation ||
        g_ammuNationAction.status == GTA_Bunker_Ammu_Nation_Status::Queued) {
        return false;
    }

    BunkerAmmuNationCommand command{};
    command.id = g_nextAmmuNationRequestId++;
    command.type = BunkerAmmuNationCommandType::ApplyTunables;
    command.deliveryPayout = deliveryPayout;
    command.ambushChance = ambushChance;
    command.timeLimit = timeLimit;
    g_pendingAmmuNation = command;

    ++g_ammuNationAction.revision;
    g_ammuNationAction.requestId = command.id;
    g_ammuNationAction.status = GTA_Bunker_Ammu_Nation_Status::Queued;
    g_ammuNationAction.detail = "Ammu-Nation tunables queued for the GTA game thread.";
    return true;
}

bool RequestTriggerExcessWeaponParts() noexcept
{
    std::scoped_lock lock(g_stateMutex);
    if (g_pendingAmmuNation ||
        g_ammuNationAction.status == GTA_Bunker_Ammu_Nation_Status::Queued) {
        return false;
    }

    BunkerAmmuNationCommand command{};
    command.id = g_nextAmmuNationRequestId++;
    command.type = BunkerAmmuNationCommandType::TriggerExcessWeaponParts;
    g_pendingAmmuNation = command;

    ++g_ammuNationAction.revision;
    g_ammuNationAction.requestId = command.id;
    g_ammuNationAction.status = GTA_Bunker_Ammu_Nation_Status::Queued;
    g_ammuNationAction.detail = "Excess Weapon Parts trigger queued for the GTA game thread.";
    return true;
}

GTA_Bunker_Ammu_Nation_Snapshot BunkerAmmuNationSnapshot()
{
    std::scoped_lock lock(g_stateMutex);
    return g_ammuNationSnapshot;
}

GTA_Bunker_Ammu_Nation_Action_Snapshot BunkerAmmuNationActionSnapshot()
{
    std::scoped_lock lock(g_stateMutex);
    return g_ammuNationAction;
}

const char* GTA_Bunker_Ammu_Nation_Status_Name(
    GTA_Bunker_Ammu_Nation_Status status) noexcept
{
    switch (status) {
    case GTA_Bunker_Ammu_Nation_Status::Idle: return "IDLE";
    case GTA_Bunker_Ammu_Nation_Status::Queued: return "QUEUED";
    case GTA_Bunker_Ammu_Nation_Status::Succeeded: return "SUCCEEDED";
    case GTA_Bunker_Ammu_Nation_Status::RuntimeUnavailable: return "RUNTIME UNAVAILABLE";
    case GTA_Bunker_Ammu_Nation_Status::UnsupportedBuild: return "UNSUPPORTED BUILD";
    case GTA_Bunker_Ammu_Nation_Status::PlayerUnavailable: return "PLAYER UNAVAILABLE";
    case GTA_Bunker_Ammu_Nation_Status::InvalidValue: return "INVALID VALUE";
    case GTA_Bunker_Ammu_Nation_Status::Failed: return "FAILED";
    default: return "UNKNOWN";
    }
}

void TickBunkerExtension() noexcept
{
    bool instantSellPending = false;
    std::optional<BunkerTunablesCommand> tunablesCommand;
    std::optional<BunkerAmmuNationCommand> ammuNationCommand;

    {
        std::scoped_lock lock(g_stateMutex);
        instantSellPending = g_requestPending;
        g_requestPending = false;

        if (g_pendingTunables) {
            tunablesCommand = std::move(g_pendingTunables);
            g_pendingTunables.reset();
        }

        if (g_pendingAmmuNation) {
            ammuNationCommand = std::move(g_pendingAmmuNation);
            g_pendingAmmuNation.reset();
        }
    }

    if (instantSellPending) {
        auto result = CompleteBunkerSellMission();
        std::scoped_lock lock(g_stateMutex);
        ++g_snapshot.revision;
        g_snapshot.status = result.status;
        g_snapshot.detail = std::move(result.detail);
    }

    if (tunablesCommand) {
        auto result = ApplyBunkerTunables(*tunablesCommand);
        std::scoped_lock lock(g_stateMutex);
        if (g_tunablesAction.requestId == tunablesCommand->id) {
            ++g_tunablesAction.revision;
            g_tunablesAction.status = result.status;
            g_tunablesAction.detail = std::move(result.detail);
        }
    }

    if (ammuNationCommand) {
        auto result = ExecuteBunkerAmmuNationCommand(*ammuNationCommand);
        std::scoped_lock lock(g_stateMutex);
        if (g_ammuNationAction.requestId == ammuNationCommand->id) {
            ++g_ammuNationAction.revision;
            g_ammuNationAction.status = result.status;
            g_ammuNationAction.detail = std::move(result.detail);
        }
    }

    bool refreshTunables = false;
    bool refreshAmmuNation = false;
    const auto now = std::chrono::steady_clock::now();
    {
        std::scoped_lock lock(g_stateMutex);
        if (g_runtime.globals &&
            (g_runtime.lastTunablesRefresh == std::chrono::steady_clock::time_point{} ||
             now - g_runtime.lastTunablesRefresh >= BunkerTunablesRefreshInterval ||
             tunablesCommand.has_value())) {
            g_runtime.lastTunablesRefresh = now;
            refreshTunables = true;
        }

        if (g_runtime.globals &&
            (g_runtime.lastAmmuNationRefresh == std::chrono::steady_clock::time_point{} ||
             now - g_runtime.lastAmmuNationRefresh >= AmmuNationRefreshInterval ||
             ammuNationCommand.has_value())) {
            g_runtime.lastAmmuNationRefresh = now;
            refreshAmmuNation = true;
        }
    }

    if (refreshTunables) {
        auto snapshot = BuildBunkerTunablesSnapshot();
        std::scoped_lock lock(g_stateMutex);
        g_tunablesSnapshot = std::move(snapshot);
    }

    if (refreshAmmuNation) {
        auto snapshot = BuildBunkerAmmuNationSnapshot();
        std::scoped_lock lock(g_stateMutex);
        g_ammuNationSnapshot = std::move(snapshot);
    }
}
}
