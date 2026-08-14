// GTA Online Enhanced 1.73 Bunker business mod controls.
#pragma once

#include <cstdint>
#include <string>

namespace Devilz::Integrations::GTA5_Enhanced
{
class Script_Global_Manager;

enum class GTA_Bunker_Instant_Sell_Status : std::uint8_t
{
    Idle,
    Queued,
    Succeeded,
    RuntimeUnavailable,
    UnsupportedBuild,
    ScriptNotRunning,
    LocalUnavailable,
    Failed
};

struct GTA_Bunker_Instant_Sell_Snapshot
{
    std::uint64_t revision = 0;
    std::uint64_t requestId = 0;
    GTA_Bunker_Instant_Sell_Status status = GTA_Bunker_Instant_Sell_Status::Idle;
    std::string detail;
};

struct GTA_Bunker_Tunables_Snapshot
{
    bool runtimeReady = false;
    int productValue = 0;
    float nearSaleMultiplier = 0.0F;
    float farSaleMultiplier = 0.0F;
    float highDemandBonus = 0.0F;
    float highDemandMaxBonus = 0.0F;
    std::string detail = "Bunker tunables are unavailable.";
};

enum class GTA_Bunker_Tunables_Status : std::uint8_t
{
    Idle,
    Queued,
    Succeeded,
    RuntimeUnavailable,
    UnsupportedBuild,
    InvalidValue,
    Failed
};

struct GTA_Bunker_Tunables_Action_Snapshot
{
    std::uint64_t revision = 0;
    std::uint64_t requestId = 0;
    GTA_Bunker_Tunables_Status status = GTA_Bunker_Tunables_Status::Idle;
    std::string detail;
};

struct GTA_Bunker_Ammu_Nation_Snapshot
{
    bool runtimeReady = false;
    int playerIndex = -1;
    int deliveryPayout = 0;
    int ambushChance = 0;
    int timeLimit = 0;
    bool triggerBitEnabled = false;
    std::string detail = "Ammu-Nation Contract globals are unavailable.";
};

enum class GTA_Bunker_Ammu_Nation_Status : std::uint8_t
{
    Idle,
    Queued,
    Succeeded,
    RuntimeUnavailable,
    UnsupportedBuild,
    PlayerUnavailable,
    InvalidValue,
    Failed
};

struct GTA_Bunker_Ammu_Nation_Action_Snapshot
{
    std::uint64_t revision = 0;
    std::uint64_t requestId = 0;
    GTA_Bunker_Ammu_Nation_Status status = GTA_Bunker_Ammu_Nation_Status::Idle;
    std::string detail;
};

void ConfigureBunkerExtension(
    std::uintptr_t scriptThreadsStorageAddress,
    std::uint64_t buildFingerprint) noexcept;

void ConfigureBunkerGlobals(
    Script_Global_Manager* globals,
    std::uint64_t buildFingerprint) noexcept;

void ResetBunkerExtension() noexcept;

[[nodiscard]] bool RequestBunkerInstantSell() noexcept;
[[nodiscard]] GTA_Bunker_Instant_Sell_Snapshot BunkerInstantSellSnapshot();
[[nodiscard]] const char* GTA_Bunker_Instant_Sell_Status_Name(
    GTA_Bunker_Instant_Sell_Status status) noexcept;

[[nodiscard]] bool RequestBunkerTunables(
    int productValue,
    float nearSaleMultiplier,
    float farSaleMultiplier,
    float highDemandBonus,
    float highDemandMaxBonus) noexcept;
[[nodiscard]] GTA_Bunker_Tunables_Snapshot BunkerTunablesSnapshot();
[[nodiscard]] GTA_Bunker_Tunables_Action_Snapshot BunkerTunablesActionSnapshot();
[[nodiscard]] const char* GTA_Bunker_Tunables_Status_Name(
    GTA_Bunker_Tunables_Status status) noexcept;

[[nodiscard]] bool RequestBunkerAmmuNationTunables(
    int deliveryPayout,
    int ambushChance,
    int timeLimit) noexcept;
[[nodiscard]] bool RequestTriggerExcessWeaponParts() noexcept;
[[nodiscard]] GTA_Bunker_Ammu_Nation_Snapshot BunkerAmmuNationSnapshot();
[[nodiscard]] GTA_Bunker_Ammu_Nation_Action_Snapshot BunkerAmmuNationActionSnapshot();
[[nodiscard]] const char* GTA_Bunker_Ammu_Nation_Status_Name(
    GTA_Bunker_Ammu_Nation_Status status) noexcept;

void TickBunkerExtension() noexcept;
}
