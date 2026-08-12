#pragma once

namespace Devilz::Integrations::GTA5_Enhanced
{
class GTA_Native_Manager;

inline constexpr int GTA_Self_Special_Ability_Count = 5;

[[nodiscard]] int SelfWantedLevelSelection() noexcept;
void SetSelfWantedLevelSelection(int level) noexcept;
void RequestSelfWantedLevel() noexcept;
void RequestSelfSuicide() noexcept;

[[nodiscard]] bool SelfUnlimitedStamina() noexcept;
void SetSelfUnlimitedStamina(bool enabled) noexcept;

[[nodiscard]] bool SelfStealthSpeed() noexcept;
void SetSelfStealthSpeed(bool enabled) noexcept;
[[nodiscard]] float SelfStealthSpeedMultiplier() noexcept;
void SetSelfStealthSpeedMultiplier(float multiplier) noexcept;

[[nodiscard]] bool SelfSpecialAbilities() noexcept;
void SetSelfSpecialAbilities(bool enabled) noexcept;
[[nodiscard]] int SelfSpecialAbilitySelection() noexcept;
void SetSelfSpecialAbilitySelection(int index) noexcept;
[[nodiscard]] const char* SelfSpecialAbilityLabel(int index) noexcept;

[[nodiscard]] bool SelfNoIdleKick() noexcept;
void SetSelfNoIdleKick(bool enabled) noexcept;
[[nodiscard]] bool SelfNoIdleKickReady() noexcept;

void ResetSelfUtilityExtension() noexcept;
void TickSelfUtilityExtension(GTA_Native_Manager& natives) noexcept;
}
