#pragma once

#include <cstdint>

namespace Devilz::Backend
{
class LoggerService;
}

namespace Devilz::Integrations::GTA5_Enhanced
{
class GTA_Native_Manager;

enum class GTA_Vehicle_Personal_Save_Status : std::uint8_t
{
    Idle,
    Queued,
    Validating,
    OpeningGarageMenu,
    WaitingForGarageSelection,
    Completed,
    NoVehicle,
    InvalidVehicle,
    AlreadyPersonalVehicle,
    Unavailable,
    Failed
};

void ConfigureVehiclePersonalSave(
    std::uintptr_t scriptGlobalsAddress,
    std::uintptr_t programTableAddress,
    std::uintptr_t scriptThreadsStorageAddress,
    std::uintptr_t scriptVmAddress,
    std::uintptr_t isSessionStartedAddress,
    Backend::LoggerService* logger) noexcept;
void ResetVehiclePersonalSave() noexcept;

void RequestSaveCurrentVehicleToGarage() noexcept;
[[nodiscard]] GTA_Vehicle_Personal_Save_Status VehiclePersonalSaveStatus() noexcept;
[[nodiscard]] bool VehiclePersonalSaveRuntimeReady() noexcept;

// Must execute from the validated GTA game/script thread. The implementation
// drives GTA's own AM_MP_VEHICLE_REWARD garage transaction and does no work
// unless a save request is pending.
void TickVehiclePersonalSave(GTA_Native_Manager& natives) noexcept;
}
