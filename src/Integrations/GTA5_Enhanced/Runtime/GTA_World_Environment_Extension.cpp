#include "GTA_World_Environment_Extension.hpp"

#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Manager.hpp"
#include "Integrations/GTA5_Enhanced/Script/Globals/Script_Global_Manager.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

namespace Devilz::Integrations::GTA5_Enhanced
{
namespace
{
constexpr GTA_Native_Hash NetworkOverrideClockTimeHash = 0xAFD3BC0F6EBB5474ULL;
constexpr GTA_Native_Hash NetworkClearClockTimeOverrideHash = 0x99599AE2C0FDB2A1ULL;
constexpr GTA_Native_Hash SetOverrideWeatherHash = 0x88791F880F624022ULL;
constexpr GTA_Native_Hash ClearOverrideWeatherHash = 0x58A3B74F26D2B532ULL;

constexpr std::uint64_t SupportedFingerprint = 0x6A4F97F605B81000ULL;

// GTA Online Enhanced 1.73 / b1158.13 request-service globals.
constexpr std::uint32_t RequestServicesBase = 2733326U;
constexpr std::uint32_t HelicopterPickupSelectorOffset = 547U;
constexpr int ServiceRequestTrigger = 1;

struct ServiceDefinition
{
    std::uint32_t offset = 0;
    const char* name = "Unknown service";
};

constexpr std::array<ServiceDefinition, static_cast<std::size_t>(GTA_Service_Request::Count)>
    ServiceDefinitions{{
        {577U, "Mobile Operations Center"},
        {585U, "Avenger"},
        {591U, "Terrorbyte"},
        {613U, "Kosatka"},
        {626U, "Dinghy"},
        {592U, "Acid Lab"},
        {648U, "Acid Lab Bike"},
        {362U, "Bail Office Transporter"},
        {548U, "Ballistic Equipment"},
        {549U, "Ballistic Equipment Instant Equip"},
        {550U, "Ballistic Equipment Instant Remove"},
        {538U, "Ammo Drop"},
        {5832U, "RC Bandito"},
        {5833U, "RC Tank"},
        {509U, "Taxi"},
        {539U, "Boat Pickup"},
        {540U, "Helicopter Pickup"},
        {540U, "Helicopter Pickup (SuperVolito)"},
        {546U, "Bull Shark Testosterone"},
        {3579U, "Backup Helicopter"},
        {490U, "Cayo Helicopter Backup"},
        {3580U, "Airstrike"}
    }};

constexpr std::array<const char*, GTA_World_Weather_Count> WeatherLabels{
    "Clear",
    "Extra Sunny",
    "Clouds",
    "Overcast",
    "Rain",
    "Clearing",
    "Thunder",
    "Smog",
    "Foggy",
    "Xmas",
    "Snow",
    "Snow Light",
    "Blizzard",
    "Halloween",
    "Neutral",
    "Rain Halloween",
    "Snow Halloween"
};

constexpr std::array<const char*, GTA_World_Weather_Count> WeatherCodes{
    "CLEAR",
    "EXTRASUNNY",
    "CLOUDS",
    "OVERCAST",
    "RAIN",
    "CLEARING",
    "THUNDER",
    "SMOG",
    "FOGGY",
    "XMAS",
    "SNOW",
    "SNOWLIGHT",
    "BLIZZARD",
    "HALLOWEEN",
    "NEUTRAL",
    "RAIN_HALLOWEEN",
    "SNOW_HALLOWEEN"
};

std::atomic_int g_networkTimeHour{12};
std::atomic_int g_networkTimeMinute{0};
std::atomic_int g_networkTimeSecond{0};
std::atomic_bool g_setNetworkTimeRequested{false};
std::atomic_bool g_freezeNetworkTime{false};
std::atomic_bool g_clearNetworkTimeRequested{false};

std::atomic_int g_weatherSelection{0};
std::atomic_bool g_setWeatherRequested{false};
std::atomic_bool g_forceWeather{false};
std::atomic_bool g_clearWeatherRequested{false};

struct WorldRuntime
{
    Script_Global_Manager* globals = nullptr;
    std::uint64_t buildFingerprint = 0;
};

struct ServiceCommand
{
    std::uint64_t id = 0;
    GTA_Service_Request request = GTA_Service_Request::MobileOperationsCenter;
};

class ServiceRequestState final
{
public:
    [[nodiscard]] bool Queue(GTA_Service_Request request)
    {
        std::scoped_lock lock(m_mutex);
        if (m_pending || m_snapshot.status == GTA_Service_Request_Status::Queued)
            return false;

        ServiceCommand command{};
        command.id = m_nextId++;
        command.request = request;
        m_pending = command;

        ++m_snapshot.revision;
        m_snapshot.requestId = command.id;
        m_snapshot.request = request;
        m_snapshot.status = GTA_Service_Request_Status::Queued;
        m_snapshot.detail = "Queued for the GTA game thread.";
        return true;
    }

    [[nodiscard]] bool Consume(ServiceCommand& command)
    {
        std::scoped_lock lock(m_mutex);
        if (!m_pending)
            return false;

        command = *m_pending;
        m_pending.reset();
        return true;
    }

    void Complete(
        const ServiceCommand& command,
        GTA_Service_Request_Status status,
        std::string detail)
    {
        std::scoped_lock lock(m_mutex);
        if (m_snapshot.requestId != command.id)
            return;

        ++m_snapshot.revision;
        m_snapshot.status = status;
        m_snapshot.detail = std::move(detail);
    }

    [[nodiscard]] GTA_Service_Request_Snapshot Snapshot() const
    {
        std::scoped_lock lock(m_mutex);
        return m_snapshot;
    }

    void Reset()
    {
        std::scoped_lock lock(m_mutex);
        m_pending.reset();
        m_snapshot = {};
        m_nextId = 1;
    }

private:
    mutable std::mutex m_mutex;
    std::optional<ServiceCommand> m_pending;
    GTA_Service_Request_Snapshot m_snapshot{};
    std::uint64_t m_nextId = 1;
};

struct ServiceRequestResult
{
    GTA_Service_Request_Status status = GTA_Service_Request_Status::Failed;
    std::string detail;
};

WorldRuntime g_runtime{};
ServiceRequestState g_serviceRequests{};

template <typename T>
bool WriteGlobalVerified(
    Script_Global_Manager& globals,
    std::uint32_t index,
    const T& value)
{
    static_assert(std::is_trivially_copyable_v<T>);

    auto pointer = globals.Get(index).Resolve();
    if (!pointer)
        return false;

    std::memcpy(
        reinterpret_cast<void*>(pointer.Value().Address()),
        &value,
        sizeof(value));

    auto readback = globals.Get(index).Read<T>();
    if (!readback)
        return false;

    const auto actual = readback.Value();
    return std::memcmp(&actual, &value, sizeof(T)) == 0;
}

ServiceRequestResult ExecuteServiceRequest(const ServiceCommand& command)
{
    auto* globals = g_runtime.globals;
    if (!globals || !globals->Ready()) {
        return {
            GTA_Service_Request_Status::RuntimeUnavailable,
            "Script globals are not configured."};
    }

    if (g_runtime.buildFingerprint != SupportedFingerprint) {
        return {
            GTA_Service_Request_Status::UnsupportedBuild,
            "Request-service globals are not registered for this GTA build."};
    }

    const auto requestIndex = static_cast<std::size_t>(command.request);
    if (requestIndex >= ServiceDefinitions.size()) {
        return {
            GTA_Service_Request_Status::InvalidRequest,
            "The requested service is invalid."};
    }

    if (command.request == GTA_Service_Request::HelicopterPickup ||
        command.request == GTA_Service_Request::SuperVolitoPickup) {
        const int selector = command.request == GTA_Service_Request::SuperVolitoPickup
            ? 1
            : -1;
        if (!WriteGlobalVerified(
                *globals,
                RequestServicesBase + HelicopterPickupSelectorOffset,
                selector)) {
            return {
                GTA_Service_Request_Status::Failed,
                "The helicopter type selector failed readback verification."};
        }
    }

    const auto& definition = ServiceDefinitions[requestIndex];
    if (!WriteGlobalVerified(
            *globals,
            RequestServicesBase + definition.offset,
            ServiceRequestTrigger)) {
        return {
            GTA_Service_Request_Status::Failed,
            "The service request trigger failed readback verification."};
    }

    return {
        GTA_Service_Request_Status::Succeeded,
        std::string{definition.name} + " request triggered and verified."};
}

void ApplySelectedNetworkTime(GTA_Native_Manager& natives) noexcept
{
    const int hour = std::clamp(g_networkTimeHour.load(std::memory_order_acquire), 0, 23);
    const int minute = std::clamp(g_networkTimeMinute.load(std::memory_order_acquire), 0, 59);
    const int second = std::clamp(g_networkTimeSecond.load(std::memory_order_acquire), 0, 59);
    (void)natives.InvokeHash<void>(NetworkOverrideClockTimeHash, hour, minute, second);
}

const char* SelectedWeatherCode() noexcept
{
    const int index = std::clamp(
        g_weatherSelection.load(std::memory_order_acquire),
        0,
        GTA_World_Weather_Count - 1);
    return WeatherCodes[static_cast<std::size_t>(index)];
}

void ApplySelectedWeather(GTA_Native_Manager& natives) noexcept
{
    (void)natives.InvokeHash<void>(SetOverrideWeatherHash, SelectedWeatherCode());
}
}

void ConfigureWorldEnvironmentExtension(
    Script_Global_Manager* globals,
    std::uint64_t buildFingerprint) noexcept
{
    g_runtime.globals = globals;
    g_runtime.buildFingerprint = buildFingerprint;
    g_serviceRequests.Reset();
}

bool RequestWorldService(GTA_Service_Request request)
{
    return g_serviceRequests.Queue(request);
}

GTA_Service_Request_Snapshot WorldServiceRequestSnapshot()
{
    return g_serviceRequests.Snapshot();
}

const char* GTA_Service_Request_Name(GTA_Service_Request request) noexcept
{
    const auto index = static_cast<std::size_t>(request);
    return index < ServiceDefinitions.size()
        ? ServiceDefinitions[index].name
        : "Unknown service";
}

const char* GTA_Service_Request_Status_Name(GTA_Service_Request_Status status) noexcept
{
    switch (status) {
    case GTA_Service_Request_Status::Idle: return "IDLE";
    case GTA_Service_Request_Status::Queued: return "QUEUED";
    case GTA_Service_Request_Status::Succeeded: return "SUCCEEDED";
    case GTA_Service_Request_Status::RuntimeUnavailable: return "RUNTIME UNAVAILABLE";
    case GTA_Service_Request_Status::UnsupportedBuild: return "UNSUPPORTED BUILD";
    case GTA_Service_Request_Status::InvalidRequest: return "INVALID REQUEST";
    case GTA_Service_Request_Status::Failed: return "FAILED";
    default: return "UNKNOWN";
    }
}

void SetNetworkTimeSelection(int hour, int minute, int second) noexcept
{
    g_networkTimeHour.store(std::clamp(hour, 0, 23), std::memory_order_release);
    g_networkTimeMinute.store(std::clamp(minute, 0, 59), std::memory_order_release);
    g_networkTimeSecond.store(std::clamp(second, 0, 59), std::memory_order_release);
}

int NetworkTimeHour() noexcept
{
    return g_networkTimeHour.load(std::memory_order_acquire);
}

int NetworkTimeMinute() noexcept
{
    return g_networkTimeMinute.load(std::memory_order_acquire);
}

int NetworkTimeSecond() noexcept
{
    return g_networkTimeSecond.load(std::memory_order_acquire);
}

void RequestSetNetworkTime() noexcept
{
    g_setNetworkTimeRequested.store(true, std::memory_order_release);
}

void SetFreezeNetworkTime(bool enabled) noexcept
{
    const bool previous = g_freezeNetworkTime.exchange(enabled, std::memory_order_acq_rel);
    if (previous && !enabled)
        g_clearNetworkTimeRequested.store(true, std::memory_order_release);
}

bool FreezeNetworkTime() noexcept
{
    return g_freezeNetworkTime.load(std::memory_order_acquire);
}

void SetWorldWeatherSelection(int index) noexcept
{
    g_weatherSelection.store(
        std::clamp(index, 0, GTA_World_Weather_Count - 1),
        std::memory_order_release);
}

int WorldWeatherSelection() noexcept
{
    return g_weatherSelection.load(std::memory_order_acquire);
}

const char* WorldWeatherLabel(int index) noexcept
{
    if (index < 0 || index >= GTA_World_Weather_Count)
        return "Unknown";
    return WeatherLabels[static_cast<std::size_t>(index)];
}

void RequestSetWorldWeather() noexcept
{
    g_setWeatherRequested.store(true, std::memory_order_release);
}

void SetForceWorldWeather(bool enabled) noexcept
{
    const bool previous = g_forceWeather.exchange(enabled, std::memory_order_acq_rel);
    if (previous && !enabled)
        g_clearWeatherRequested.store(true, std::memory_order_release);
}

bool ForceWorldWeather() noexcept
{
    return g_forceWeather.load(std::memory_order_acquire);
}

void RequestResetWorldWeather() noexcept
{
    g_forceWeather.store(false, std::memory_order_release);
    g_setWeatherRequested.store(false, std::memory_order_release);
    g_clearWeatherRequested.store(true, std::memory_order_release);
}

void ResetWorldEnvironmentExtension() noexcept
{
    g_runtime = {};
    g_serviceRequests.Reset();

    g_networkTimeHour.store(12, std::memory_order_release);
    g_networkTimeMinute.store(0, std::memory_order_release);
    g_networkTimeSecond.store(0, std::memory_order_release);
    g_setNetworkTimeRequested.store(false, std::memory_order_release);
    g_freezeNetworkTime.store(false, std::memory_order_release);
    g_clearNetworkTimeRequested.store(false, std::memory_order_release);

    g_weatherSelection.store(0, std::memory_order_release);
    g_setWeatherRequested.store(false, std::memory_order_release);
    g_forceWeather.store(false, std::memory_order_release);
    g_clearWeatherRequested.store(false, std::memory_order_release);
}

void TickWorldEnvironmentExtension(GTA_Native_Manager& natives) noexcept
{
    ServiceCommand serviceCommand{};
    if (g_serviceRequests.Consume(serviceCommand)) {
        auto result = ExecuteServiceRequest(serviceCommand);
        g_serviceRequests.Complete(
            serviceCommand,
            result.status,
            std::move(result.detail));
    }

    if (g_clearNetworkTimeRequested.exchange(false, std::memory_order_acq_rel))
        (void)natives.InvokeHash<void>(NetworkClearClockTimeOverrideHash);

    if (g_setNetworkTimeRequested.exchange(false, std::memory_order_acq_rel))
        ApplySelectedNetworkTime(natives);

    if (g_freezeNetworkTime.load(std::memory_order_acquire))
        ApplySelectedNetworkTime(natives);

    if (g_clearWeatherRequested.exchange(false, std::memory_order_acq_rel))
        (void)natives.InvokeHash<void>(ClearOverrideWeatherHash);

    if (g_setWeatherRequested.exchange(false, std::memory_order_acq_rel))
        ApplySelectedWeather(natives);

    if (g_forceWeather.load(std::memory_order_acquire))
        ApplySelectedWeather(natives);
}
}
