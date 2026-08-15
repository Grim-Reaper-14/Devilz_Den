#pragma once

#include <cstdint>

namespace Devilz::Backend
{
class LoggerService;
}

namespace Devilz::Integrations::GTA5_Enhanced
{
enum class Devils_Aimbot_Status : std::uint8_t
{
    Uninitialized,
    Ready,
    UnsupportedBuild,
    PatternFailure,
    PatchFailure
};

bool ConfigureDevilsAimbot(Backend::LoggerService& logger, std::uint64_t buildFingerprint);
void ResetDevilsAimbot();

[[nodiscard]] Devils_Aimbot_Status DevilsAimbotStatus() noexcept;
[[nodiscard]] const char* DevilsAimbotStatusText() noexcept;
[[nodiscard]] bool DevilsAimbotAvailable() noexcept;
[[nodiscard]] bool DevilsAimbotEnabled() noexcept;
[[nodiscard]] bool DevilsAimbotAimForHead() noexcept;
[[nodiscard]] bool DevilsAimbotTargetDrivers() noexcept;

bool SetDevilsAimbotEnabled(bool enabled);
bool SetDevilsAimbotAimForHead(bool enabled);
bool SetDevilsAimbotTargetDrivers(bool enabled);
}
