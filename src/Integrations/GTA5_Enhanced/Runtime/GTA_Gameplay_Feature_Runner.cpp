#include "GTA_Gameplay_Feature_Runner.hpp"

#include "Backend/Logging/LoggerService.hpp"
#include "GTA_Gameplay_State.hpp"
#include "GTA_Teleport_Locations.hpp"
#include "GTA_Vehicle_Catalog.hpp"
#include "GTA_Vehicle_State.hpp"
#include "GTA_Weapon_Catalog.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Manager.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Registry.hpp"

#include <cmath>
#include <limits>
#include <string>
#include <string_view>
#include <utility>

namespace Devilz::Integrations::GTA5_Enhanced
{
namespace
{
constexpr std::uint32_t MaxGroundAttempts = 40;
constexpr std::uint32_t MaxVehicleStreamAttempts = 180;
constexpr float GroundProbeZ = 1000.0F;
constexpr float Pi = 3.14159265358979323846F;

[[nodiscard]] std::string CopyLocalized(GTA_Native_Manager& natives, const char* label, std::string fallback)
{
    if (!label || *label == '\0')
        return fallback;
    const auto localized = natives.Invoke<const char*>(GTA_Native_Id::GetFilenameForAudioConversation, label);
    if (localized && *localized && **localized != '\0' && std::string_view(*localized) != "NULL")
        return std::string(*localized);
    return std::string(label);
}
}

void GTA_Gameplay_Feature_Runner::Configure(GTA_Native_Manager& natives, Backend::LoggerService& logger) noexcept
{
    m_natives = &natives;
    m_logger = &logger;
}

void GTA_Gameplay_Feature_Runner::Reset() noexcept
{
    m_natives = nullptr;
    m_logger = nullptr;
    m_godModeApplied = false;
    m_neverWantedApplied = false;
    m_infiniteOxygenApplied = false;
    m_noRagdollApplied = false;
    m_infiniteAmmoApplied = false;
    m_vehicleCatalogIndex = 0;
    m_vehicleSpawnPhase = Vehicle_Spawn_Phase::Idle;
    m_vehicleSpawnModel = 0;
    m_vehicleSpawnOptions = {};
    m_vehicleSpawnCoords = {};
    m_vehicleSpawnHeading = 0.0F;
    m_vehicleSpawnHandle = 0;
    m_vehicleStreamAttempts = 0;
    GTA_Vehicle_State::Instance().Reset();
    m_teleportPhase = Teleport_Phase::Idle;
    m_waypoint = {};
    m_groundAttempts = 0;
}

void GTA_Gameplay_Feature_Runner::Tick() noexcept
{
    if (!m_natives || !m_natives->Ready())
        return;
    TickMenuInputSuppression();
    TickGodMode();
    TickNeverWanted();
    TickSuperJump();
    TickInfiniteOxygen();
    TickNoRagdoll();
    TickKeepPlayerClean();
    TickInfiniteAmmo();
    TickWeaponActions();
    TickVehicleCatalog();
    TickVehicleSpawner();
    TickVehicleForge();
    TickPresetTeleport();
    TickTeleportToWaypoint();
}

void GTA_Gameplay_Feature_Runner::TickMenuInputSuppression() noexcept
{
    if (GTA_Gameplay_State::Instance().MenuInputCaptured())
        (void)m_natives->Invoke<void>(GTA_Native_Id::DisableAllControlActions, 0);
}

void GTA_Gameplay_Feature_Runner::TickGodMode() noexcept
{
    auto& state = GTA_Gameplay_State::Instance();
    const bool desired = state.GodMode();
    if (!desired && !m_godModeApplied)
        return;
    const auto ped = m_natives->Invoke<int>(GTA_Native_Id::PlayerPedId);
    if (!ped || *ped == 0)
        return;
    if (!m_natives->Invoke<void>(GTA_Native_Id::SetEntityInvincible, *ped, desired, true))
        return;
    if (desired != m_godModeApplied && m_logger)
        m_logger->Log(Backend::LogLevel::Info, std::string("God Mode ") + (desired ? "enabled" : "disabled") + " on the game thread", "GTA5_Enhanced.Features");
    m_godModeApplied = desired;
}

void GTA_Gameplay_Feature_Runner::TickNeverWanted() noexcept
{
    auto& state = GTA_Gameplay_State::Instance();
    const bool desired = state.NeverWanted();
    if (desired) {
        const auto player = m_natives->Invoke<int>(GTA_Native_Id::PlayerId);
        if (!player)
            return;
        const bool setLevel = m_natives->Invoke<void>(GTA_Native_Id::SetPlayerWantedLevel, *player, 0, false);
        const bool applyNow = m_natives->Invoke<void>(GTA_Native_Id::SetPlayerWantedLevelNow, *player, false);
        const bool clampMax = m_natives->Invoke<void>(GTA_Native_Id::SetMaxWantedLevel, 0);
        if (!setLevel || !applyNow || !clampMax)
            return;
        if (!m_neverWantedApplied && m_logger)
            m_logger->Log(Backend::LogLevel::Info, "Never Wanted enabled on the game thread", "GTA5_Enhanced.Features");
        m_neverWantedApplied = true;
        return;
    }
    if (!m_neverWantedApplied || !m_natives->Invoke<void>(GTA_Native_Id::SetMaxWantedLevel, 6))
        return;
    m_neverWantedApplied = false;
    if (m_logger)
        m_logger->Log(Backend::LogLevel::Info, "Never Wanted disabled; max wanted level restored", "GTA5_Enhanced.Features");
}

void GTA_Gameplay_Feature_Runner::TickSuperJump() noexcept
{
    if (!GTA_Gameplay_State::Instance().SuperJump())
        return;
    const auto player = m_natives->Invoke<int>(GTA_Native_Id::PlayerId);
    if (player)
        (void)m_natives->Invoke<void>(GTA_Native_Id::SetSuperJumpThisFrame, *player);
}

void GTA_Gameplay_Feature_Runner::TickInfiniteOxygen() noexcept
{
    auto& state = GTA_Gameplay_State::Instance();
    const bool desired = state.InfiniteOxygen();
    if (!desired && !m_infiniteOxygenApplied)
        return;
    const auto ped = m_natives->Invoke<int>(GTA_Native_Id::PlayerPedId);
    if (!ped || *ped == 0)
        return;
    const float maxTime = desired ? static_cast<float>((std::numeric_limits<int>::max)()) : -1.0F;
    if (!m_natives->Invoke<void>(GTA_Native_Id::SetPedMaxTimeUnderwater, *ped, maxTime))
        return;
    if (desired != m_infiniteOxygenApplied && m_logger)
        m_logger->Log(Backend::LogLevel::Info, desired ? "Infinite Oxygen enabled" : "Infinite Oxygen disabled; underwater timer restored", "GTA5_Enhanced.Features");
    m_infiniteOxygenApplied = desired;
}

void GTA_Gameplay_Feature_Runner::TickNoRagdoll() noexcept
{
    auto& state = GTA_Gameplay_State::Instance();
    const bool desired = state.NoRagdoll();
    if (!desired && !m_noRagdollApplied)
        return;
    const auto ped = m_natives->Invoke<int>(GTA_Native_Id::PlayerPedId);
    if (!ped || *ped == 0)
        return;
    if (!m_natives->Invoke<void>(GTA_Native_Id::SetPedCanRagdoll, *ped, !desired))
        return;
    if (desired != m_noRagdollApplied && m_logger)
        m_logger->Log(Backend::LogLevel::Info, desired ? "No Ragdoll enabled" : "No Ragdoll disabled; ragdoll restored", "GTA5_Enhanced.Features");
    m_noRagdollApplied = desired;
}

void GTA_Gameplay_Feature_Runner::TickKeepPlayerClean() noexcept
{
    if (!GTA_Gameplay_State::Instance().KeepPlayerClean())
        return;
    const auto ped = m_natives->Invoke<int>(GTA_Native_Id::PlayerPedId);
    if (!ped || *ped == 0)
        return;
    (void)m_natives->Invoke<void>(GTA_Native_Id::ClearPedBloodDamage, *ped);
    (void)m_natives->Invoke<void>(GTA_Native_Id::ClearPedWetness, *ped);
    (void)m_natives->Invoke<void>(GTA_Native_Id::ClearPedEnvDirt, *ped);
    (void)m_natives->Invoke<void>(GTA_Native_Id::ResetPedVisibleDamage, *ped);
}

void GTA_Gameplay_Feature_Runner::TickInfiniteAmmo() noexcept
{
    auto& state = GTA_Gameplay_State::Instance();
    const bool desired = state.InfiniteAmmo();
    if (!desired && !m_infiniteAmmoApplied)
        return;
    const auto ped = m_natives->Invoke<int>(GTA_Native_Id::PlayerPedId);
    if (!ped || *ped == 0)
        return;
    if (!m_natives->Invoke<void>(GTA_Native_Id::SetPedInfiniteAmmo, *ped, desired, 0U))
        return;
    if (desired != m_infiniteAmmoApplied && m_logger)
        m_logger->Log(Backend::LogLevel::Info, desired ? "Infinite Ammo enabled" : "Infinite Ammo disabled", "GTA5_Enhanced.Features");
    m_infiniteAmmoApplied = desired;
}

void GTA_Gameplay_Feature_Runner::TickWeaponActions() noexcept
{
    auto& state = GTA_Gameplay_State::Instance();
    const bool giveAll = state.ConsumeGiveAllWeaponsRequest();
    const bool giveAmmo = state.ConsumeGiveMaxAmmoRequest();
    if (!giveAll && !giveAmmo)
        return;
    const auto ped = m_natives->Invoke<int>(GTA_Native_Id::PlayerPedId);
    if (!ped || *ped == 0)
        return;
    bool success = true;
    for (const auto name : GTA_All_Weapon_Names) {
        const auto hash = GTA_Weapon_Hash(name);
        if (giveAll)
            success = m_natives->Invoke<void>(GTA_Native_Id::GiveWeaponToPed, *ped, hash, 9999, false, false) && success;
        if (giveAmmo)
            success = m_natives->Invoke<void>(GTA_Native_Id::SetPedAmmo, *ped, hash, 9999, false) && success;
    }
    if (m_logger) {
        if (giveAll)
            m_logger->Log(success ? Backend::LogLevel::Info : Backend::LogLevel::Warning, success ? "Give All Weapons completed" : "Give All Weapons completed with native failures", "GTA5_Enhanced.Features");
        if (giveAmmo)
            m_logger->Log(success ? Backend::LogLevel::Info : Backend::LogLevel::Warning, success ? "Give Max Ammo completed" : "Give Max Ammo completed with native failures", "GTA5_Enhanced.Features");
    }
}

void GTA_Gameplay_Feature_Runner::TickVehicleCatalog() noexcept
{
    if (m_vehicleCatalogIndex >= GTA_Vehicle_Model_Names.size())
        return;
    const auto modelName = GTA_Vehicle_Model_Names[m_vehicleCatalogIndex++];
    const auto modelHash = GTA_Model_Hash(modelName);
    const auto valid = m_natives->Invoke<bool>(GTA_Native_Id::IsModelInCdimage, modelHash);
    if (!valid || !*valid)
        return;

    GTA_Vehicle_Metadata metadata{};
    metadata.modelHash = modelHash;
    metadata.modelName = std::string(modelName);
    metadata.displayName = metadata.modelName;
    const auto displayLabel = m_natives->Invoke<const char*>(GTA_Native_Id::GetDisplayNameFromVehicleModel, modelHash);
    if (displayLabel && *displayLabel)
        metadata.displayName = CopyLocalized(*m_natives, *displayLabel, metadata.modelName);
    const auto makeLabel = m_natives->Invoke<const char*>(GTA_Native_Id::GetMakeNameFromVehicleModel, modelHash);
    if (makeLabel && *makeLabel)
        metadata.makeName = CopyLocalized(*m_natives, *makeLabel, {});
    const auto vehicleClass = m_natives->Invoke<int>(GTA_Native_Id::GetVehicleClassFromName, modelHash);
    if (vehicleClass)
        metadata.vehicleClass = *vehicleClass;
    GTA_Vehicle_State::Instance().PublishMetadata(std::move(metadata));
}

void GTA_Gameplay_Feature_Runner::BeginVehicleSpawn(std::uint32_t modelHash, const GTA_Vehicle_Spawn_Options& options) noexcept
{
    m_vehicleSpawnModel = modelHash;
    m_vehicleSpawnOptions = options;
    m_vehicleSpawnCoords = {};
    m_vehicleSpawnHeading = 0.0F;
    m_vehicleSpawnHandle = 0;
    m_vehicleStreamAttempts = 0;
    m_vehicleSpawnPhase = Vehicle_Spawn_Phase::Validate;
    GTA_Vehicle_State::Instance().SetSpawnStatus(GTA_Vehicle_Spawn_Status::Validating);
}

void GTA_Gameplay_Feature_Runner::TickVehicleSpawner() noexcept
{
    auto& state = GTA_Vehicle_State::Instance();
    if (m_vehicleSpawnPhase == Vehicle_Spawn_Phase::Idle) {
        std::uint32_t modelHash = 0;
        GTA_Vehicle_Spawn_Options options{};
        if (state.ConsumeSpawnRequest(modelHash, options))
            BeginVehicleSpawn(modelHash, options);
    }
    if (m_vehicleSpawnPhase == Vehicle_Spawn_Phase::Idle)
        return;

    if (m_vehicleSpawnPhase == Vehicle_Spawn_Phase::Validate) {
        const auto valid = m_natives->Invoke<bool>(GTA_Native_Id::IsModelInCdimage, m_vehicleSpawnModel);
        if (!valid || !*valid) {
            FinishVehicleSpawn(false, "Vehicle spawn failed: model is not present in the Enhanced streaming image");
            return;
        }
        const auto ped = m_natives->Invoke<int>(GTA_Native_Id::PlayerPedId);
        if (!ped || *ped == 0) {
            FinishVehicleSpawn(false, "Vehicle spawn failed: player ped unavailable");
            return;
        }
        const auto coords = m_natives->Invoke<GTA_Native_Script_Vector>(GTA_Native_Id::GetEntityCoords, *ped, false);
        const auto heading = m_natives->Invoke<float>(GTA_Native_Id::GetEntityHeading, *ped);
        if (!coords || !heading) {
            FinishVehicleSpawn(false, "Vehicle spawn failed: player transform unavailable");
            return;
        }
        m_vehicleSpawnCoords = *coords;
        m_vehicleSpawnHeading = *heading;
        const float radians = m_vehicleSpawnHeading * (Pi / 180.0F);
        m_vehicleSpawnCoords.x -= std::sin(radians) * 5.0F;
        m_vehicleSpawnCoords.y += std::cos(radians) * 5.0F;
        m_vehicleSpawnCoords.z += 1.0F;
        m_vehicleSpawnPhase = Vehicle_Spawn_Phase::Stream;
        state.SetSpawnStatus(GTA_Vehicle_Spawn_Status::Streaming);
    }

    if (m_vehicleSpawnPhase == Vehicle_Spawn_Phase::Stream) {
        (void)m_natives->Invoke<void>(GTA_Native_Id::RequestModel, m_vehicleSpawnModel);
        const auto loaded = m_natives->Invoke<bool>(GTA_Native_Id::HasModelLoaded, m_vehicleSpawnModel);
        if (loaded && *loaded) {
            m_vehicleSpawnPhase = Vehicle_Spawn_Phase::Create;
            state.SetSpawnStatus(GTA_Vehicle_Spawn_Status::Creating);
        } else if (++m_vehicleStreamAttempts >= MaxVehicleStreamAttempts) {
            FinishVehicleSpawn(false, "Vehicle spawn failed: model streaming timed out");
            return;
        } else {
            return;
        }
    }

    if (m_vehicleSpawnPhase == Vehicle_Spawn_Phase::Create) {
        const auto vehicle = m_natives->Invoke<int>(GTA_Native_Id::CreateVehicle, m_vehicleSpawnModel,
            m_vehicleSpawnCoords.x, m_vehicleSpawnCoords.y, m_vehicleSpawnCoords.z,
            m_vehicleSpawnHeading, false, false, false);
        if (!vehicle || *vehicle == 0) {
            FinishVehicleSpawn(false, "Vehicle spawn failed: CREATE_VEHICLE returned no handle");
            return;
        }
        m_vehicleSpawnHandle = *vehicle;
        m_vehicleSpawnPhase = Vehicle_Spawn_Phase::Apply;
        state.SetSpawnStatus(GTA_Vehicle_Spawn_Status::Applying);
    }

    if (m_vehicleSpawnPhase != Vehicle_Spawn_Phase::Apply)
        return;
    bool success = true;
    if (m_vehicleSpawnOptions.placeOnGround) {
        const auto grounded = m_natives->Invoke<bool>(GTA_Native_Id::SetVehicleOnGroundProperly, m_vehicleSpawnHandle, 5.0F);
        success = grounded && *grounded && success;
    }
    if (m_vehicleSpawnOptions.spawnMaxed) {
        success = m_natives->Invoke<void>(GTA_Native_Id::SetVehicleModKit, m_vehicleSpawnHandle, 0) && success;
        for (int slot = 0; slot < 50; ++slot) {
            const auto count = m_natives->Invoke<int>(GTA_Native_Id::GetNumVehicleMods, m_vehicleSpawnHandle, slot);
            if (count && *count > 0)
                success = m_natives->Invoke<void>(GTA_Native_Id::SetVehicleMod, m_vehicleSpawnHandle, slot, *count - 1, false) && success;
        }
        success = m_natives->Invoke<void>(GTA_Native_Id::ToggleVehicleMod, m_vehicleSpawnHandle, 18, true) && success;
        success = m_natives->Invoke<void>(GTA_Native_Id::ToggleVehicleMod, m_vehicleSpawnHandle, 22, true) && success;
    }
    if (m_vehicleSpawnOptions.engineRunning)
        success = m_natives->Invoke<void>(GTA_Native_Id::SetVehicleEngineOn, m_vehicleSpawnHandle, true, true, false) && success;
    if (m_vehicleSpawnOptions.clean)
        success = m_natives->Invoke<void>(GTA_Native_Id::SetVehicleDirtLevel, m_vehicleSpawnHandle, 0.0F) && success;
    if (m_vehicleSpawnOptions.invincible)
        success = m_natives->Invoke<void>(GTA_Native_Id::SetEntityInvincible, m_vehicleSpawnHandle, true, true) && success;
    if (m_vehicleSpawnOptions.spawnInside) {
        const auto ped = m_natives->Invoke<int>(GTA_Native_Id::PlayerPedId);
        success = ped && *ped != 0 && m_natives->Invoke<void>(GTA_Native_Id::SetPedIntoVehicle, *ped, m_vehicleSpawnHandle, -1) && success;
    }
    state.SetLastSpawnedVehicle(m_vehicleSpawnHandle);
    FinishVehicleSpawn(success, success ? "Vehicle spawned successfully" : "Vehicle spawned with one or more option failures");
}

void GTA_Gameplay_Feature_Runner::FinishVehicleSpawn(bool success, const char* detail) noexcept
{
    if (m_vehicleSpawnModel != 0)
        (void)m_natives->Invoke<void>(GTA_Native_Id::SetModelAsNoLongerNeeded, m_vehicleSpawnModel);
    GTA_Vehicle_State::Instance().SetSpawnStatus(success ? GTA_Vehicle_Spawn_Status::Succeeded : GTA_Vehicle_Spawn_Status::Failed);
    m_vehicleSpawnPhase = Vehicle_Spawn_Phase::Idle;
    m_vehicleSpawnModel = 0;
    m_vehicleSpawnHandle = 0;
    m_vehicleStreamAttempts = 0;
    if (m_logger)
        m_logger->Log(success ? Backend::LogLevel::Info : Backend::LogLevel::Warning,
            detail ? detail : (success ? "Vehicle spawn succeeded" : "Vehicle spawn failed"), "GTA5_Enhanced.Features");
}

int GTA_Gameplay_Feature_Runner::CurrentVehicle() noexcept
{
    const auto ped = m_natives->Invoke<int>(GTA_Native_Id::PlayerPedId);
    if (!ped || *ped == 0)
        return 0;
    const auto vehicle = m_natives->Invoke<int>(GTA_Native_Id::GetVehiclePedIsIn, *ped, false);
    return vehicle ? *vehicle : 0;
}

void GTA_Gameplay_Feature_Runner::TickVehicleForge() noexcept
{
    const auto command = GTA_Vehicle_State::Instance().ConsumeForgeCommand();
    if (command.type == GTA_Vehicle_Forge_Command_Type::None)
        return;
    const int vehicle = CurrentVehicle();
    if (vehicle == 0) {
        if (m_logger)
            m_logger->Log(Backend::LogLevel::Warning, "Devils Forge ignored: player is not inside a vehicle", "GTA5_Enhanced.Features");
        return;
    }

    bool success = true;
    switch (command.type) {
    case GTA_Vehicle_Forge_Command_Type::SetMod:
        success = m_natives->Invoke<void>(GTA_Native_Id::SetVehicleModKit, vehicle, 0) &&
                  m_natives->Invoke<void>(GTA_Native_Id::SetVehicleMod, vehicle, command.arg0, command.arg1, command.arg2 != 0);
        break;
    case GTA_Vehicle_Forge_Command_Type::ToggleMod:
        success = m_natives->Invoke<void>(GTA_Native_Id::SetVehicleModKit, vehicle, 0) &&
                  m_natives->Invoke<void>(GTA_Native_Id::ToggleVehicleMod, vehicle, command.arg0, command.arg1 != 0);
        break;
    case GTA_Vehicle_Forge_Command_Type::SetWheelType:
        success = m_natives->Invoke<void>(GTA_Native_Id::SetVehicleWheelType, vehicle, command.arg0);
        break;
    case GTA_Vehicle_Forge_Command_Type::SetPrimaryPaint:
        success = m_natives->Invoke<void>(GTA_Native_Id::SetVehicleModColor1, vehicle, command.arg0, command.arg1, command.arg2);
        break;
    case GTA_Vehicle_Forge_Command_Type::SetSecondaryPaint:
        success = m_natives->Invoke<void>(GTA_Native_Id::SetVehicleModColor2, vehicle, command.arg0, command.arg1);
        break;
    case GTA_Vehicle_Forge_Command_Type::SetExtraColours:
        success = m_natives->Invoke<void>(GTA_Native_Id::SetVehicleExtraColours, vehicle, command.arg0, command.arg1);
        break;
    case GTA_Vehicle_Forge_Command_Type::SetCustomPrimaryRgb:
        success = m_natives->Invoke<void>(GTA_Native_Id::SetVehicleCustomPrimaryColour, vehicle,
            std::clamp(command.arg0, 0, 255), std::clamp(command.arg1, 0, 255), std::clamp(command.arg2, 0, 255));
        break;
    case GTA_Vehicle_Forge_Command_Type::SetCustomSecondaryRgb:
        success = m_natives->Invoke<void>(GTA_Native_Id::SetVehicleCustomSecondaryColour, vehicle,
            std::clamp(command.arg0, 0, 255), std::clamp(command.arg1, 0, 255), std::clamp(command.arg2, 0, 255));
        break;
    case GTA_Vehicle_Forge_Command_Type::SetLoweredStance:
        success = m_natives->Invoke<void>(GTA_Native_Id::SetReducedSuspensionForce, vehicle, command.arg0 != 0);
        break;
    case GTA_Vehicle_Forge_Command_Type::CleanVehicle:
        success = m_natives->Invoke<void>(GTA_Native_Id::SetVehicleDirtLevel, vehicle, 0.0F);
        break;
    default:
        break;
    }
    if (!success && m_logger)
        m_logger->Log(Backend::LogLevel::Warning, "Devils Forge native command failed", "GTA5_Enhanced.Features");
}

void GTA_Gameplay_Feature_Runner::TickPresetTeleport() noexcept
{
    auto& state = GTA_Gameplay_State::Instance();
    const auto id = state.ConsumeTeleportToLocationRequest();
    if (id == GTA_Teleport_Location_Id::None)
        return;
    const auto* location = FindTeleportLocation(id);
    if (!location) {
        if (m_logger)
            m_logger->Log(Backend::LogLevel::Warning, "Preset teleport ignored: unknown location id", "GTA5_Enhanced.Features");
        return;
    }
    const bool moved = TeleportPlayer(location->x, location->y, location->z);
    if (m_logger)
        m_logger->Log(moved ? Backend::LogLevel::Info : Backend::LogLevel::Warning,
            std::string("Preset teleport ") + (moved ? "succeeded: " : "failed: ") + std::string(location->label), "GTA5_Enhanced.Features");
}

void GTA_Gameplay_Feature_Runner::TickTeleportToWaypoint() noexcept
{
    auto& state = GTA_Gameplay_State::Instance();
    if (m_teleportPhase == Teleport_Phase::Idle && state.ConsumeTeleportToWaypointRequest())
        BeginTeleportToWaypoint();
    if (m_teleportPhase != Teleport_Phase::ResolveGround)
        return;
    state.SetTeleportStatus(GTA_Teleport_Waypoint_Status::Resolving);
    (void)m_natives->Invoke<void>(GTA_Native_Id::RequestCollisionAtCoord, m_waypoint.x, m_waypoint.y, m_waypoint.z);
    float groundZ = m_waypoint.z;
    const auto foundGround = m_natives->Invoke<bool>(GTA_Native_Id::GetGroundZFor3DCoord, m_waypoint.x, m_waypoint.y, GroundProbeZ, &groundZ, false, false);
    if (foundGround && *foundGround) {
        const bool moved = TeleportPlayer(m_waypoint.x, m_waypoint.y, groundZ + 1.0F);
        FinishTeleport(moved, moved ? "Teleport to waypoint succeeded using exact ground height" : "Teleport failed: entity move invocation failed");
        return;
    }
    ++m_groundAttempts;
    if (m_groundAttempts < MaxGroundAttempts)
        return;
    float waterHeight = 0.0F;
    const auto foundWater = m_natives->Invoke<bool>(GTA_Native_Id::GetWaterHeight, m_waypoint.x, m_waypoint.y, m_waypoint.z, &waterHeight);
    if (foundWater && *foundWater) {
        const bool moved = TeleportPlayer(m_waypoint.x, m_waypoint.y, waterHeight + 1.0F);
        FinishTeleport(moved, moved ? "Teleport to waypoint succeeded using water height fallback" : "Teleport failed: water fallback move failed");
        return;
    }
    const auto approxHeight = m_natives->Invoke<float>(GTA_Native_Id::GetApproxHeightForPoint, m_waypoint.x, m_waypoint.y);
    if (approxHeight) {
        const bool moved = TeleportPlayer(m_waypoint.x, m_waypoint.y, *approxHeight + 1.0F);
        FinishTeleport(moved, moved ? "Teleport to waypoint succeeded using approximate terrain fallback" : "Teleport failed: approximate terrain fallback move failed");
        return;
    }
    FinishTeleport(false, "Teleport failed: no exact ground, water, or approximate terrain height was available");
}

void GTA_Gameplay_Feature_Runner::BeginTeleportToWaypoint() noexcept
{
    auto& state = GTA_Gameplay_State::Instance();
    const auto active = m_natives->Invoke<bool>(GTA_Native_Id::IsWaypointActive);
    if (!active || !*active) {
        state.SetTeleportStatus(GTA_Teleport_Waypoint_Status::NoWaypoint);
        if (m_logger)
            m_logger->Log(Backend::LogLevel::Warning, "Teleport to waypoint ignored: no active waypoint", "GTA5_Enhanced.Features");
        return;
    }
    const auto waypointSprite = m_natives->Invoke<int>(GTA_Native_Id::GetWaypointBlipEnumId);
    if (!waypointSprite) {
        FinishTeleport(false, "Teleport failed: waypoint blip sprite could not be resolved");
        return;
    }
    const auto blip = m_natives->Invoke<int>(GTA_Native_Id::GetClosestBlipInfoId, *waypointSprite);
    if (!blip || *blip == 0) {
        FinishTeleport(false, "Teleport failed: waypoint blip handle could not be resolved");
        return;
    }
    const auto coords = m_natives->Invoke<GTA_Native_Script_Vector>(GTA_Native_Id::GetBlipCoords, *blip);
    if (!coords) {
        FinishTeleport(false, "Teleport failed: waypoint coordinates could not be read");
        return;
    }
    m_waypoint = *coords;
    m_groundAttempts = 0;
    m_teleportPhase = Teleport_Phase::ResolveGround;
    state.SetTeleportStatus(GTA_Teleport_Waypoint_Status::Resolving);
    if (m_logger)
        m_logger->Log(Backend::LogLevel::Info, "Teleport to waypoint queued for terrain resolution", "GTA5_Enhanced.Features");
}

bool GTA_Gameplay_Feature_Runner::TeleportPlayer(float x, float y, float z) noexcept
{
    const auto ped = m_natives->Invoke<int>(GTA_Native_Id::PlayerPedId);
    if (!ped || *ped == 0)
        return false;
    int entity = *ped;
    const auto vehicle = m_natives->Invoke<int>(GTA_Native_Id::GetVehiclePedIsIn, *ped, false);
    if (vehicle && *vehicle != 0)
        entity = *vehicle;
    return m_natives->Invoke<void>(GTA_Native_Id::SetEntityCoordsNoOffset, entity, x, y, z, true, true, true);
}

void GTA_Gameplay_Feature_Runner::FinishTeleport(bool success, const char* detail) noexcept
{
    GTA_Gameplay_State::Instance().SetTeleportStatus(success ? GTA_Teleport_Waypoint_Status::Succeeded : GTA_Teleport_Waypoint_Status::Failed);
    m_teleportPhase = Teleport_Phase::Idle;
    m_waypoint = {};
    m_groundAttempts = 0;
    if (m_logger)
        m_logger->Log(success ? Backend::LogLevel::Info : Backend::LogLevel::Warning,
            detail ? detail : (success ? "Teleport succeeded" : "Teleport failed"), "GTA5_Enhanced.Features");
}
}
