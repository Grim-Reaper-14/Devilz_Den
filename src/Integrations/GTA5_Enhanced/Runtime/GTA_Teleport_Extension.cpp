#include "GTA_Teleport_Extension.hpp"

#include "GTA_Gameplay_State.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Manager.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Registry.hpp"

#include <array>
#include <atomic>
#include <cmath>

namespace Devilz::Integrations::GTA5_Enhanced
{
namespace
{
constexpr int ClothesStoreSprite = 73;
constexpr int TattooShopSprite = 75;
constexpr int GunShopSprite = 110;
constexpr int CarMeetSprite = 777;
constexpr int ArenaWorkshopSprite = 643;

// Matches YimMenuV2's current Teleport to Objective search order.
constexpr std::array<int, 17> ObjectiveSprites{
    1,   // RADAR_LEVEL
    0,   // RADAR_HIGHER
    2,   // RADAR_LOWER
    143, // RADAR_OBJECTIVE_BLUE
    144, // RADAR_OBJECTIVE_GREEN
    145, // RADAR_OBJECTIVE_RED
    146, // RADAR_OBJECTIVE_YELLOW
    478, // RADAR_CONTRABAND
    535, // RADAR_TARGET_A
    536, // RADAR_TARGET_B
    537, // RADAR_TARGET_C
    538, // RADAR_TARGET_D
    539, // RADAR_TARGET_E
    540, // RADAR_TARGET_F
    541, // RADAR_TARGET_G
    542, // RADAR_TARGET_H
    549  // RADAR_PICKUP_MACHINEGUN
};

constexpr float WaypointChangeThreshold = 0.5F;

std::atomic_bool g_autoTeleportToWaypoint{false};
std::atomic<GTA_Teleport_Quick_Destination> g_quickDestination{
    GTA_Teleport_Quick_Destination::None};
std::atomic<GTA_Teleport_Quick_Status> g_quickStatus{GTA_Teleport_Quick_Status::Idle};

// Game-thread-only waypoint edge detection. Keeping the map waypoint intact is
// friendlier than forcibly clearing it, while still preventing repeated TPs.
bool g_waypointSeen = false;
GTA_Native_Script_Vector g_lastWaypoint{};

[[nodiscard]] bool TeleportBusy() noexcept
{
    const auto status = GTA_Gameplay_State::Instance().TeleportStatus();
    return status == GTA_Teleport_Waypoint_Status::Queued ||
           status == GTA_Teleport_Waypoint_Status::Resolving;
}

[[nodiscard]] bool GetBlipLocation(
    GTA_Native_Manager& natives,
    int sprite,
    GTA_Native_Script_Vector& location) noexcept
{
    const auto blip = natives.Invoke<int>(GTA_Native_Id::GetClosestBlipInfoId, sprite);
    if (!blip || *blip == 0)
        return false;

    const auto coords = natives.Invoke<GTA_Native_Script_Vector>(GTA_Native_Id::GetBlipCoords, *blip);
    if (!coords)
        return false;

    location = *coords;
    location.z += 1.0F;
    return true;
}

[[nodiscard]] bool GetObjectiveLocation(
    GTA_Native_Manager& natives,
    GTA_Native_Script_Vector& location) noexcept
{
    for (const int sprite : ObjectiveSprites) {
        if (GetBlipLocation(natives, sprite, location))
            return true;
    }
    return false;
}

[[nodiscard]] bool GetQuickDestinationLocation(
    GTA_Native_Manager& natives,
    GTA_Teleport_Quick_Destination destination,
    GTA_Native_Script_Vector& location) noexcept
{
    switch (destination) {
    case GTA_Teleport_Quick_Destination::Objective:
        return GetObjectiveLocation(natives, location);
    case GTA_Teleport_Quick_Destination::LsCarMeet:
        return GetBlipLocation(natives, CarMeetSprite, location);
    case GTA_Teleport_Quick_Destination::ArenaGarage:
        return GetBlipLocation(natives, ArenaWorkshopSprite, location);
    case GTA_Teleport_Quick_Destination::ClothingShop:
        return GetBlipLocation(natives, ClothesStoreSprite, location);
    case GTA_Teleport_Quick_Destination::GunShop:
        return GetBlipLocation(natives, GunShopSprite, location);
    case GTA_Teleport_Quick_Destination::TattooShop:
        return GetBlipLocation(natives, TattooShopSprite, location);
    default:
        return false;
    }
}

[[nodiscard]] bool TeleportCurrentEntity(
    GTA_Native_Manager& natives,
    const GTA_Native_Script_Vector& location) noexcept
{
    const auto ped = natives.Invoke<int>(GTA_Native_Id::PlayerPedId);
    if (!ped || *ped == 0)
        return false;

    int entity = *ped;
    const auto seated = natives.Invoke<bool>(GTA_Native_Id::IsPedInAnyVehicle, *ped, false);
    if (seated && *seated) {
        const auto vehicle = natives.Invoke<int>(GTA_Native_Id::GetVehiclePedIsIn, *ped, false);
        if (vehicle && *vehicle != 0)
            entity = *vehicle;
    }

    return natives.Invoke<void>(
        GTA_Native_Id::SetEntityCoordsNoOffset,
        entity,
        location.x,
        location.y,
        location.z,
        true,
        true,
        true);
}

[[nodiscard]] bool WaypointChanged(const GTA_Native_Script_Vector& current) noexcept
{
    if (!g_waypointSeen)
        return true;

    return std::fabs(current.x - g_lastWaypoint.x) > WaypointChangeThreshold ||
           std::fabs(current.y - g_lastWaypoint.y) > WaypointChangeThreshold ||
           std::fabs(current.z - g_lastWaypoint.z) > WaypointChangeThreshold;
}

void TickQuickTeleport(GTA_Native_Manager& natives) noexcept
{
    const auto destination = g_quickDestination.exchange(
        GTA_Teleport_Quick_Destination::None,
        std::memory_order_acq_rel);
    if (destination == GTA_Teleport_Quick_Destination::None)
        return;

    if (TeleportBusy()) {
        g_quickDestination.store(destination, std::memory_order_release);
        return;
    }

    GTA_Native_Script_Vector location{};
    if (!GetQuickDestinationLocation(natives, destination, location)) {
        g_quickStatus.store(GTA_Teleport_Quick_Status::NoDestination, std::memory_order_release);
        return;
    }

    const bool moved = TeleportCurrentEntity(natives, location);
    g_quickStatus.store(
        moved ? GTA_Teleport_Quick_Status::Succeeded : GTA_Teleport_Quick_Status::Failed,
        std::memory_order_release);
}

void TickAutoTeleportToWaypoint(GTA_Native_Manager& natives) noexcept
{
    if (!g_autoTeleportToWaypoint.load(std::memory_order_acquire)) {
        g_waypointSeen = false;
        g_lastWaypoint = {};
        return;
    }

    const auto active = natives.Invoke<bool>(GTA_Native_Id::IsWaypointActive);
    if (!active || !*active) {
        g_waypointSeen = false;
        g_lastWaypoint = {};
        return;
    }

    const auto waypointSprite = natives.Invoke<int>(GTA_Native_Id::GetWaypointBlipEnumId);
    if (!waypointSprite)
        return;

    const auto blip = natives.Invoke<int>(GTA_Native_Id::GetClosestBlipInfoId, *waypointSprite);
    if (!blip || *blip == 0)
        return;

    const auto coords = natives.Invoke<GTA_Native_Script_Vector>(GTA_Native_Id::GetBlipCoords, *blip);
    if (!coords || !WaypointChanged(*coords))
        return;

    if (TeleportBusy())
        return;

    g_lastWaypoint = *coords;
    g_waypointSeen = true;
    GTA_Gameplay_State::Instance().RequestTeleportToWaypoint();
}
}

void SetAutoTeleportToWaypoint(bool enabled) noexcept
{
    g_autoTeleportToWaypoint.store(enabled, std::memory_order_release);
}

bool AutoTeleportToWaypoint() noexcept
{
    return g_autoTeleportToWaypoint.load(std::memory_order_acquire);
}

void RequestTeleportToObjective() noexcept
{
    RequestQuickTeleport(GTA_Teleport_Quick_Destination::Objective);
}

void RequestQuickTeleport(GTA_Teleport_Quick_Destination destination) noexcept
{
    if (destination == GTA_Teleport_Quick_Destination::None)
        return;

    g_quickStatus.store(GTA_Teleport_Quick_Status::Queued, std::memory_order_release);
    g_quickDestination.store(destination, std::memory_order_release);
}

GTA_Teleport_Quick_Status QuickTeleportStatus() noexcept
{
    return g_quickStatus.load(std::memory_order_acquire);
}

void TickTeleportExtension(GTA_Native_Manager& natives) noexcept
{
    TickQuickTeleport(natives);
    TickAutoTeleportToWaypoint(natives);
}
}
