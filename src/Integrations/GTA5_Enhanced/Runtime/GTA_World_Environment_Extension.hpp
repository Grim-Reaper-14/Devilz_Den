#pragma once

#include <cstdint>
#include <string>

namespace Devilz::Integrations::GTA5_Enhanced
{
class GTA_Native_Manager;
class Script_Global_Manager;

inline constexpr int GTA_World_Weather_Count = 17;

enum class GTA_Service_Request : std::uint8_t
{
    MobileOperationsCenter,
    Avenger,
    Terrorbyte,
    Kosatka,
    Dinghy,
    AcidLab,
    AcidLabBike,
    BailOfficeTransporter,
    BallisticEquipment,
    BallisticEquipmentEquip,
    BallisticEquipmentRemove,
    AmmoDrop,
    RcBandito,
    RcTank,
    Taxi,
    BoatPickup,
    HelicopterPickup,
    SuperVolitoPickup,
    BullSharkTestosterone,
    BackupHelicopter,
    CayoHelicopterBackup,
    Airstrike,
    Count
};

enum class GTA_Service_Request_Status : std::uint8_t
{
    Idle,
    Queued,
    Succeeded,
    RuntimeUnavailable,
    UnsupportedBuild,
    InvalidRequest,
    Failed
};

struct GTA_Service_Request_Snapshot
{
    std::uint64_t revision = 0;
    std::uint64_t requestId = 0;
    GTA_Service_Request request = GTA_Service_Request::MobileOperationsCenter;
    GTA_Service_Request_Status status = GTA_Service_Request_Status::Idle;
    std::string detail;
};

void ConfigureWorldEnvironmentExtension(
    Script_Global_Manager* globals,
    std::uint64_t buildFingerprint) noexcept;

[[nodiscard]] bool RequestWorldService(GTA_Service_Request request);
[[nodiscard]] GTA_Service_Request_Snapshot WorldServiceRequestSnapshot();
[[nodiscard]] const char* GTA_Service_Request_Name(GTA_Service_Request request) noexcept;
[[nodiscard]] const char* GTA_Service_Request_Status_Name(
    GTA_Service_Request_Status status) noexcept;

void SetNetworkTimeSelection(int hour, int minute, int second) noexcept;
[[nodiscard]] int NetworkTimeHour() noexcept;
[[nodiscard]] int NetworkTimeMinute() noexcept;
[[nodiscard]] int NetworkTimeSecond() noexcept;
void RequestSetNetworkTime() noexcept;
void SetFreezeNetworkTime(bool enabled) noexcept;
[[nodiscard]] bool FreezeNetworkTime() noexcept;

void SetWorldWeatherSelection(int index) noexcept;
[[nodiscard]] int WorldWeatherSelection() noexcept;
[[nodiscard]] const char* WorldWeatherLabel(int index) noexcept;
void RequestSetWorldWeather() noexcept;
void SetForceWorldWeather(bool enabled) noexcept;
[[nodiscard]] bool ForceWorldWeather() noexcept;
void RequestResetWorldWeather() noexcept;

void ResetWorldEnvironmentExtension() noexcept;
void TickWorldEnvironmentExtension(GTA_Native_Manager& natives) noexcept;
}
