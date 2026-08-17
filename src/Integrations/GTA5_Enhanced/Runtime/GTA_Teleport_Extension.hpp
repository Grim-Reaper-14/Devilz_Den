#pragma once

#include <cstdint>

namespace Devilz::Integrations::GTA5_Enhanced
{
class GTA_Native_Manager;

enum class GTA_Teleport_Quick_Status : std::uint8_t
{
    Idle,
    Queued,
    Succeeded,
    NoDestination,
    Failed
};

enum class GTA_Teleport_Quick_Destination : std::uint8_t
{
    None,
    Objective,
    LsCarMeet,
    ArenaGarage,
    ClothingShop,
    GunShop,
    TattooShop
};

void SetAutoTeleportToWaypoint(bool enabled) noexcept;
[[nodiscard]] bool AutoTeleportToWaypoint() noexcept;

void RequestTeleportToObjective() noexcept;
void RequestQuickTeleport(GTA_Teleport_Quick_Destination destination) noexcept;
[[nodiscard]] GTA_Teleport_Quick_Status QuickTeleportStatus() noexcept;
[[nodiscard]] bool TeleportExtensionHasWork() noexcept;

// Serviced from the existing validated game-thread/native tick. Quick
// destinations use live map blips; waypoint automation reuses the existing
// terrain-resolving waypoint request path in GTA_Gameplay_Feature_Runner.
void TickTeleportExtension(GTA_Native_Manager& natives) noexcept;
}
