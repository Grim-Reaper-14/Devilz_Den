#include "GTA_World_Environment_Extension.hpp"

#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Manager.hpp"

#include <algorithm>
#include <array>
#include <atomic>

namespace Devilz::Integrations::GTA5_Enhanced
{
namespace
{
constexpr GTA_Native_Hash NetworkOverrideClockTimeHash = 0xAFD3BC0F6EBB5474ULL;
constexpr GTA_Native_Hash NetworkClearClockTimeOverrideHash = 0x99599AE2C0FDB2A1ULL;
constexpr GTA_Native_Hash SetOverrideWeatherHash = 0x88791F880F624022ULL;
constexpr GTA_Native_Hash ClearOverrideWeatherHash = 0x58A3B74F26D2B532ULL;

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
