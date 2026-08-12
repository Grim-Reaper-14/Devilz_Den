#pragma once

#include <cstdint>

namespace Devilz::Integrations::GTA5_Enhanced
{
class GTA_Native_Manager;

enum class GTA_Personal_Vehicle_Request_Status : std::uint8_t
{
    Idle,
    Queued,
    Requesting,
    Requested,
    Unavailable,
    Busy,
    Failed
};

void RequestCurrentPersonalVehicle() noexcept;
[[nodiscard]] GTA_Personal_Vehicle_Request_Status PersonalVehicleRequestStatus() noexcept;

void ResetSelfOnlineExtension() noexcept;
void TickSelfOnlineExtension(GTA_Native_Manager& natives) noexcept;
}
