#pragma once

#include "GTA_Vehicle_State.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Call_Context.hpp"

#include <cstddef>
#include <cstdint>

namespace Devilz::Backend
{
class LoggerService;
}

namespace Devilz::Integrations::GTA5_Enhanced
{
class GTA_Native_Manager;

class GTA_Gameplay_Feature_Runner final
{
public:
    void Configure(GTA_Native_Manager& natives, Backend::LoggerService& logger) noexcept;
    void Reset() noexcept;
    void Tick() noexcept;

private:
    enum class Teleport_Phase : std::uint8_t
    {
        Idle,
        ResolveGround
    };

    enum class Vehicle_Spawn_Phase : std::uint8_t
    {
        Idle,
        Validate,
        Stream,
        Create,
        Apply
    };

    void TickMenuInputSuppression() noexcept;
    void TickGodMode() noexcept;
    void TickNeverWanted() noexcept;
    void TickSuperJump() noexcept;
    void TickInfiniteOxygen() noexcept;
    void TickNoRagdoll() noexcept;
    void TickKeepPlayerClean() noexcept;
    void TickInfiniteAmmo() noexcept;
    void TickWeaponActions() noexcept;
    void TickVehicleCatalog() noexcept;
    void TickVehicleSpawner() noexcept;
    void TickVehicleForge() noexcept;
    void TickVehicleMaintenance() noexcept;
    void TickVehicleForgeSnapshot() noexcept;
    void BuildVehicleForgeSnapshot(int vehicle) noexcept;
    void BeginVehicleSpawn(std::uint32_t modelHash, const GTA_Vehicle_Spawn_Options& options) noexcept;
    void FinishVehicleSpawn(bool success, const char* detail) noexcept;
    [[nodiscard]] int CurrentVehicle() noexcept;
    void TickTeleportToWaypoint() noexcept;
    void TickPresetTeleport() noexcept;
    void BeginTeleportToWaypoint() noexcept;
    [[nodiscard]] bool TeleportPlayer(float x, float y, float z) noexcept;
    void FinishTeleport(bool success, const char* detail) noexcept;

    GTA_Native_Manager* m_natives = nullptr;
    Backend::LoggerService* m_logger = nullptr;
    bool m_godModeApplied = false;
    bool m_neverWantedApplied = false;
    bool m_infiniteOxygenApplied = false;
    bool m_noRagdollApplied = false;
    bool m_infiniteAmmoApplied = false;

    std::size_t m_vehicleCatalogIndex = 0;
    Vehicle_Spawn_Phase m_vehicleSpawnPhase = Vehicle_Spawn_Phase::Idle;
    std::uint32_t m_vehicleSpawnModel = 0;
    GTA_Vehicle_Spawn_Options m_vehicleSpawnOptions{};
    GTA_Native_Script_Vector m_vehicleSpawnCoords{};
    float m_vehicleSpawnHeading = 0.0F;
    int m_vehicleSpawnHandle = 0;
    std::uint32_t m_vehicleStreamAttempts = 0;
    int m_forgeSnapshotVehicle = 0;
    int m_vehicleGodModeAppliedVehicle = 0;

    Teleport_Phase m_teleportPhase = Teleport_Phase::Idle;
    GTA_Native_Script_Vector m_waypoint{};
    std::uint32_t m_groundAttempts = 0;
};
}
