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

enum class GTA_Random_Event_Id : std::uint8_t
{
    DrugVehicle,
    MovieProps,
    SleepingGuard,
    ExoticExports,
    Slasher,
    PhantomCar,
    Sightseeing,
    SmugglerTrail,
    Cerberus,
    SmugglerPlane,
    CrimeScene,
    MetalDetector,
    Convoy,
    Robbery,
    XmasMugger,
    BankShootout,
    ArmouredTruck,
    PossessedAnimals,
    Ghosthunt,
    XmasTruck,
    CommunityOutreach,
    GetawayDriver,
    SurvivalGrouping,
    ValentineCheater,
    Count
};

inline constexpr std::size_t GTA_Random_Event_Count =
    static_cast<std::size_t>(GTA_Random_Event_Id::Count);

enum class GTA_Random_Event_State : std::uint8_t
{
    Inactive,
    Available,
    Active,
    Cleanup,
    Unknown
};

enum class GTA_Random_Event_Action_Status : std::uint8_t
{
    Unavailable,
    Ready,
    Queued,
    RequestSent,
    Succeeded,
    Failed,
    AlreadyActive,
    NotActive,
    CoordinatesUnavailable
};

struct GTA_Random_Event_Info
{
    GTA_Random_Event_State state = GTA_Random_Event_State::Unknown;
    int subvariation = 0;
    int maxSubvariation = 29;
    int fmmcType = 0;
    int inactiveTimeMs = 0;
    int availableTimeMs = 0;
    int remainingTimeMs = 0;
    float triggerX = 0.0F;
    float triggerY = 0.0F;
    float triggerZ = 0.0F;
    float triggerRange = 0.0F;
    bool hasCoordinates = false;
    bool scriptRunning = false;
};

struct GTA_Random_Events_Snapshot
{
    bool runtimeReady = false;
    bool freemodeRunning = false;
    bool clientInitialized = false;
    int activeEventCount = 0;
    std::array<GTA_Random_Event_Info, GTA_Random_Event_Count> events{};
    GTA_Random_Event_Action_Status actionStatus = GTA_Random_Event_Action_Status::Unavailable;
    std::string runtimeDetail = "Random Events runtime is unavailable.";
    std::string actionDetail;
};

[[nodiscard]] const char* GTA_Random_Event_Name(GTA_Random_Event_Id event) noexcept;
[[nodiscard]] GTA_Random_Events_Snapshot GetRandomEventsSnapshot();

void SelectRandomEvent(GTA_Random_Event_Id event) noexcept;
[[nodiscard]] bool RequestLaunchRandomEvent(
    GTA_Random_Event_Id event,
    int subvariation) noexcept;
[[nodiscard]] bool RequestKillRandomEvent(GTA_Random_Event_Id event) noexcept;
[[nodiscard]] bool RequestTeleportToRandomEvent(GTA_Random_Event_Id event) noexcept;
[[nodiscard]] bool RequestSetRandomEventCooldown(
    GTA_Random_Event_Id event,
    int milliseconds) noexcept;
[[nodiscard]] bool RequestSetRandomEventAvailability(
    GTA_Random_Event_Id event,
    int milliseconds) noexcept;

void ConfigureRandomEventsExtension(
    std::uintptr_t scriptGlobalsAddress,
    std::uintptr_t programTableAddress,
    std::uintptr_t scriptThreadsStorageAddress,
    std::uintptr_t scriptVmAddress,
    std::uint64_t buildFingerprint,
    Backend::LoggerService* logger) noexcept;

void ResetRandomEventsExtension() noexcept;
void TickRandomEventsExtension(GTA_Native_Manager& natives) noexcept;
}
