#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Devilz::Backend
{
class LoggerService;
}

namespace Devilz::Integrations::GTA5_Enhanced
{
class GTA_Native_Manager;

struct GTA_Personal_Vehicle_Info
{
    int id = -1;
    std::uint32_t modelHash = 0;
    std::string displayName;
    std::string plateText;
};

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

enum class GTA_Personal_Vehicle_List_Status : std::uint8_t
{
    Idle,
    Queued,
    Scanning,
    Ready,
    Unavailable,
    Failed
};

void ConfigureSelfOnlineExtensionLogging(Backend::LoggerService* logger) noexcept;

void RequestCurrentPersonalVehicle() noexcept;
void RequestPersonalVehicle(int personalVehicleId) noexcept;
[[nodiscard]] GTA_Personal_Vehicle_Request_Status PersonalVehicleRequestStatus() noexcept;

void RequestPersonalVehicleListRefresh() noexcept;
[[nodiscard]] GTA_Personal_Vehicle_List_Status PersonalVehicleListStatus() noexcept;
[[nodiscard]] std::vector<GTA_Personal_Vehicle_Info> PersonalVehicleSnapshot();
[[nodiscard]] std::uint64_t PersonalVehicleGeneration() noexcept;

void ResetSelfOnlineExtension() noexcept;
void TickSelfOnlineExtension(GTA_Native_Manager& natives) noexcept;
}
