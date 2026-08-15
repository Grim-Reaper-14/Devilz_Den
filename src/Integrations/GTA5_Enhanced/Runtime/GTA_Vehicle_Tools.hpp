#pragma once

#include <cstdint>

namespace Devilz::Backend
{
class LoggerService;
}

namespace Devilz::Integrations::GTA5_Enhanced
{
class GTA_Native_Manager;

enum class GTA_Vehicle_Clone_Nearest_Status : std::uint8_t
{
    Idle,
    Queued,
    Searching,
    SpawnQueued,
    Succeeded,
    NoVehicle,
    Unavailable,
    Failed
};

void ConfigureVehicleToolsLogging(Backend::LoggerService* logger) noexcept;
void RequestCloneNearestVehicle() noexcept;
[[nodiscard]] GTA_Vehicle_Clone_Nearest_Status CloneNearestVehicleStatus() noexcept;
void ResetVehicleTools() noexcept;
void TickVehicleTools(GTA_Native_Manager& natives) noexcept;
}
