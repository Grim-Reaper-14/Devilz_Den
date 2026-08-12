#pragma once

#include <cstdint>

namespace Devilz::Integrations::GTA5_Enhanced
{
class GTA_Native_Manager;
class GTA_Vehicle_State;

enum class GTA_Vehicle_Garage_Status : std::uint8_t
{
    Idle,
    Queued,
    Saving,
    Saved,
    OwnershipRequired,
    Failed
};

enum class GTA_Garage_Save_Result : std::uint8_t
{
    Success,
    NoVehicle,
    NotOwned,
    InvalidVehicle,
    OwnershipRegistrationRequired,
    Failed
};

void RequestVehicleGarageSave() noexcept;
[[nodiscard]] GTA_Vehicle_Garage_Status VehicleGarageSaveStatus() noexcept;
[[nodiscard]] GTA_Garage_Save_Result SaveCurrentForgeVehicleToGarage(
    GTA_Native_Manager& natives,
    GTA_Vehicle_State& state) noexcept;
void TickVehicleGarageSave(GTA_Native_Manager& natives) noexcept;
}
