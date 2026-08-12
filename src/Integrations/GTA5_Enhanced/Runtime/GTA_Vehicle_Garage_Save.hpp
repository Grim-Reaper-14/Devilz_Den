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

void RequestVehicleLSCPrep() noexcept;
[[nodiscard]] GTA_Vehicle_LSC_Prep_Status VehicleLSCPrepStatus() noexcept;

// Kept behind the existing validated game-thread tick wiring so this change
// does not require another hook. The implementation now performs LSC prep only.
void TickVehicleGarageSave(GTA_Native_Manager& natives) noexcept;
}
