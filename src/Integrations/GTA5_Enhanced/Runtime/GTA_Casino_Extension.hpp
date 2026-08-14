#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace Devilz::Backend
{
class LoggerService;
}

namespace Devilz::Integrations::GTA5_Enhanced
{
class GTA_Native_Manager;
class Script_Global_Manager;

// GTA Online Enhanced 1.73 / b1158.13 Lucky Wheel layout.
inline constexpr std::uint64_t GTA_Casino_Supported_Fingerprint = 0x6A4F97F605B81000ULL;
inline constexpr std::uint32_t GTA_Lucky_Wheel_Tunables_Base = 262145U;
inline constexpr std::uint32_t GTA_Lucky_Wheel_Max_Spins_Global =
    GTA_Lucky_Wheel_Tunables_Base + 26855U;
inline constexpr std::uint32_t GTA_Lucky_Wheel_Additional_Spins_Global =
    GTA_Lucky_Wheel_Tunables_Base + 26856U;
inline constexpr std::uint32_t GTA_Lucky_Wheel_Gta_Plus_Max_Spins_Global =
    GTA_Lucky_Wheel_Tunables_Base + 37458U;

inline constexpr std::uint32_t GTA_Lucky_Wheel_Outcome_Array_Local = 150U;
inline constexpr std::uint32_t GTA_Lucky_Wheel_Outcome_Player_Stride = 5U;
inline constexpr int GTA_Lucky_Wheel_Outcome_Count = 20;

[[nodiscard]] constexpr std::uint32_t GTA_Lucky_Wheel_Outcome_Local_Index(
    std::uint32_t player) noexcept
{
    // Local 150 is the SCR_ARRAY size header. Player zero's first field begins
    // at 151, followed by five slots per player.
    return GTA_Lucky_Wheel_Outcome_Array_Local + 1U +
        player * GTA_Lucky_Wheel_Outcome_Player_Stride;
}

enum class GTA_Lucky_Wheel_Reward_Kind : std::uint8_t
{
    Clothing,
    Reputation,
    Cash,
    Chips,
    VehicleDiscount,
    Mystery,
    PodiumVehicle
};

struct GTA_Lucky_Wheel_Outcome_Definition
{
    GTA_Lucky_Wheel_Reward_Kind kind = GTA_Lucky_Wheel_Reward_Kind::Clothing;
    int amount = 0;
    const char* name = "Unknown";
};

// b1158.13 casino_lucky_wheel reward classification and amount tables.
inline constexpr std::array<GTA_Lucky_Wheel_Outcome_Definition,
                            GTA_Lucky_Wheel_Outcome_Count>
    GTA_Lucky_Wheel_Outcomes{{
        {GTA_Lucky_Wheel_Reward_Kind::Clothing, 0, "Clothing #1"},
        {GTA_Lucky_Wheel_Reward_Kind::Reputation, 2500, "2,500 RP"},
        {GTA_Lucky_Wheel_Reward_Kind::Cash, 20000, "$20,000 Cash"},
        {GTA_Lucky_Wheel_Reward_Kind::Chips, 10000, "10,000 Chips"},
        {GTA_Lucky_Wheel_Reward_Kind::VehicleDiscount, 0, "Vehicle Discount"},
        {GTA_Lucky_Wheel_Reward_Kind::Reputation, 5000, "5,000 RP"},
        {GTA_Lucky_Wheel_Reward_Kind::Cash, 30000, "$30,000 Cash"},
        {GTA_Lucky_Wheel_Reward_Kind::Chips, 15000, "15,000 Chips"},
        {GTA_Lucky_Wheel_Reward_Kind::Clothing, 0, "Clothing #2"},
        {GTA_Lucky_Wheel_Reward_Kind::Reputation, 7500, "7,500 RP"},
        {GTA_Lucky_Wheel_Reward_Kind::Chips, 20000, "20,000 Chips"},
        {GTA_Lucky_Wheel_Reward_Kind::Mystery, 0, "Mystery Prize"},
        {GTA_Lucky_Wheel_Reward_Kind::Clothing, 0, "Clothing #3"},
        {GTA_Lucky_Wheel_Reward_Kind::Reputation, 10000, "10,000 RP"},
        {GTA_Lucky_Wheel_Reward_Kind::Cash, 40000, "$40,000 Cash"},
        {GTA_Lucky_Wheel_Reward_Kind::Chips, 25000, "25,000 Chips"},
        {GTA_Lucky_Wheel_Reward_Kind::Clothing, 0, "Clothing #4"},
        {GTA_Lucky_Wheel_Reward_Kind::Reputation, 15000, "15,000 RP"},
        {GTA_Lucky_Wheel_Reward_Kind::PodiumVehicle, 0, "Podium Vehicle"},
        {GTA_Lucky_Wheel_Reward_Kind::Cash, 50000, "$50,000 Cash"}
    }};

[[nodiscard]] constexpr const char* GTA_Lucky_Wheel_Outcome_Name(int outcome) noexcept
{
    return outcome >= 0 && outcome < GTA_Lucky_Wheel_Outcome_Count
        ? GTA_Lucky_Wheel_Outcomes[static_cast<std::size_t>(outcome)].name
        : "Not selected";
}

enum class GTA_Lucky_Wheel_Settings_Status : std::uint8_t
{
    Idle,
    Queued,
    Succeeded,
    RuntimeUnavailable,
    UnsupportedBuild,
    Failed
};

struct GTA_Casino_Snapshot
{
    std::uint64_t revision = 0;
    bool buildSupported = false;
    bool globalsReady = false;
    bool scriptThreadsReady = false;
    bool luckyWheelScriptActive = false;
    bool outcomeLocalReady = false;

    int requestedMaxSpins = 1;
    bool requestedAdditionalSpins = true;
    int requestedGtaPlusMaxSpins = 2;

    bool liveSettingsAvailable = false;
    int liveMaxSpins = 1;
    bool liveAdditionalSpins = false;
    int liveGtaPlusMaxSpins = 2;

    int selectedOutcome = 18;
    int currentOutcome = -1;
    std::uint32_t currentOutcomeLocal = 0;
    bool forceOutcome = false;

    GTA_Lucky_Wheel_Settings_Status settingsStatus =
        GTA_Lucky_Wheel_Settings_Status::Idle;
    std::string settingsDetail;
    std::string runtimeDetail;
};

void ConfigureCasinoExtension(
    Script_Global_Manager* globals,
    std::uintptr_t scriptThreadsStorageAddress,
    std::uint64_t buildFingerprint,
    Backend::LoggerService* logger) noexcept;

void ResetCasinoExtension() noexcept;
void TickCasinoExtension(GTA_Native_Manager& natives) noexcept;

void SetLuckyWheelSpinSettings(
    int maxSpins,
    bool additionalSpins,
    int gtaPlusMaxSpins) noexcept;
[[nodiscard]] bool RequestApplyLuckyWheelSpinSettings();

void SetLuckyWheelSelectedOutcome(int outcome) noexcept;
void SetForceLuckyWheelOutcome(bool enabled) noexcept;

[[nodiscard]] GTA_Casino_Snapshot CasinoSnapshot();
[[nodiscard]] const char* GTA_Lucky_Wheel_Settings_Status_Name(
    GTA_Lucky_Wheel_Settings_Status status) noexcept;
}
