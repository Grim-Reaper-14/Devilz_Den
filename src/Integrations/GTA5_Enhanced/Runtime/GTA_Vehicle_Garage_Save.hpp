#pragma once

#include <cstdint>

namespace Devilz::Integrations::GTA5_Enhanced
{
class GTA_Native_Manager;

enum class GTA_Vehicle_LSC_Prep_Status : std::uint8_t
{
    Idle,
    Queued,
    Preparing,
    Prepared,
    NoVehicle,
    Failed
};

enum class GTA_Vehicle_LSC_Restriction_Status : std::uint8_t
{
    Disabled,
    WaitingForScript,
    Active,
    Unsupported
};

void RequestVehicleLSCPrep() noexcept;
[[nodiscard]] GTA_Vehicle_LSC_Prep_Status VehicleLSCPrepStatus() noexcept;

void ConfigureVehicleLSCRestrictions(std::uintptr_t programTableAddress) noexcept;
void ResetVehicleLSCRestrictions() noexcept;
void SetRemoveLSCRestrictions(bool enabled) noexcept;
[[nodiscard]] bool RemoveLSCRestrictions() noexcept;
[[nodiscard]] GTA_Vehicle_LSC_Restriction_Status RemoveLSCRestrictionsStatus() noexcept;

// Kept behind the existing validated game-thread tick wiring so this change
// does not require another hook. The implementation performs LSC prep and
// services the reversible carmod_shop restriction patches.
void TickVehicleGarageSave(GTA_Native_Manager& natives) noexcept;
}
