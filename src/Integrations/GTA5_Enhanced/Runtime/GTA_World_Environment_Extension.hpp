#pragma once

namespace Devilz::Integrations::GTA5_Enhanced
{
class GTA_Native_Manager;

inline constexpr int GTA_World_Weather_Count = 17;

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
