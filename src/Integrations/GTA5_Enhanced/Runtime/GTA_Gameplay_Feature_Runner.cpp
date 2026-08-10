#include "GTA_Gameplay_Feature_Runner.hpp"

#include "Backend/Logging/LoggerService.hpp"
#include "GTA_Gameplay_State.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Manager.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Registry.hpp"

#include <string>

namespace Devilz::Integrations::GTA5_Enhanced
{
namespace
{
constexpr std::uint32_t MaxGroundAttempts = 20;
}

void GTA_Gameplay_Feature_Runner::Configure(
    GTA_Native_Manager& natives,
    Backend::LoggerService& logger) noexcept
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
    TickTeleportToWaypoint();
}

void GTA_Gameplay_Feature_Runner::TickMenuInputSuppression() noexcept
{
    if (!GTA_Gameplay_State::Instance().MenuInputCaptured())
        return;

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

    if (desired != m_godModeApplied && m_logger) {
        m_logger->Log(
            Backend::LogLevel::Info,
            std::string("God Mode ") + (desired ? "enabled" : "disabled") + " on the game thread",
            "GTA5_Enhanced.Features");
    }

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

        const bool setLevel = m_natives->Invoke<void>(
            GTA_Native_Id::SetPlayerWantedLevel,
            *player,
            0,
            false);
        const bool applyNow = m_natives->Invoke<void>(
            GTA_Native_Id::SetPlayerWantedLevelNow,
            *player,
            false);
        const bool clampMax = m_natives->Invoke<void>(GTA_Native_Id::SetMaxWantedLevel, 0);
        if (!setLevel || !applyNow || !clampMax)
            return;

        if (!m_neverWantedApplied && m_logger) {
            m_logger->Log(
                Backend::LogLevel::Info,
                "Never Wanted enabled on the game thread",
                "GTA5_Enhanced.Features");
        }
        m_neverWantedApplied = true;
        return;
    }

    if (!m_neverWantedApplied)
        return;

    if (!m_natives->Invoke<void>(GTA_Native_Id::SetMaxWantedLevel, 6))
        return;

    m_neverWantedApplied = false;
    if (m_logger) {
        m_logger->Log(
            Backend::LogLevel::Info,
            "Never Wanted disabled; max wanted level restored",
            "GTA5_Enhanced.Features");
    }
}

void GTA_Gameplay_Feature_Runner::TickTeleportToWaypoint() noexcept
{
    auto& state = GTA_Gameplay_State::Instance();

    if (m_teleportPhase == Teleport_Phase::Idle && state.ConsumeTeleportToWaypointRequest())
        BeginTeleportToWaypoint();

    if (m_teleportPhase != Teleport_Phase::ResolveGround)
        return;

    state.SetTeleportStatus(GTA_Teleport_Waypoint_Status::Resolving);

    constexpr float probeZ = 1000.0F;
    (void)m_natives->Invoke<void>(
        GTA_Native_Id::RequestCollisionAtCoord,
        m_waypoint.x,
        m_waypoint.y,
        probeZ);

    float groundZ = 0.0F;
    const auto found = m_natives->Invoke<bool>(
        GTA_Native_Id::GetGroundZFor3DCoord,
        m_waypoint.x,
        m_waypoint.y,
        probeZ,
        &groundZ,
        false,
        false);

    if (found && *found) {
        const auto ped = m_natives->Invoke<int>(GTA_Native_Id::PlayerPedId);
        if (!ped || *ped == 0) {
            FinishTeleport(false, "Teleport failed: local player ped is unavailable");
            return;
        }

        const bool moved = m_natives->Invoke<void>(
            GTA_Native_Id::SetEntityCoordsNoOffset,
            *ped,
            m_waypoint.x,
            m_waypoint.y,
            groundZ + 1.0F,
            true,
            true,
            true);

        FinishTeleport(moved, moved
            ? "Teleport to waypoint succeeded"
            : "Teleport failed: SET_ENTITY_COORDS_NO_OFFSET invocation failed");
        return;
    }

    ++m_groundAttempts;
    if (m_groundAttempts >= MaxGroundAttempts)
        FinishTeleport(false, "Teleport failed: ground height could not be resolved");
}

void GTA_Gameplay_Feature_Runner::BeginTeleportToWaypoint() noexcept
{
    auto& state = GTA_Gameplay_State::Instance();

    const auto active = m_natives->Invoke<bool>(GTA_Native_Id::IsWaypointActive);
    if (!active || !*active) {
        state.SetTeleportStatus(GTA_Teleport_Waypoint_Status::NoWaypoint);
        if (m_logger) {
            m_logger->Log(
                Backend::LogLevel::Warning,
                "Teleport to waypoint ignored: no active waypoint",
                "GTA5_Enhanced.Features");
        }
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

    if (m_logger) {
        m_logger->Log(
            Backend::LogLevel::Info,
            "Teleport to waypoint queued for ground resolution",
            "GTA5_Enhanced.Features");
    }
}

void GTA_Gameplay_Feature_Runner::FinishTeleport(bool success, const char* detail) noexcept
{
    GTA_Gameplay_State::Instance().SetTeleportStatus(
        success ? GTA_Teleport_Waypoint_Status::Succeeded : GTA_Teleport_Waypoint_Status::Failed);

    m_teleportPhase = Teleport_Phase::Idle;
    m_waypoint = {};
    m_groundAttempts = 0;

    if (m_logger) {
        m_logger->Log(
            success ? Backend::LogLevel::Info : Backend::LogLevel::Warning,
            detail ? detail : (success ? "Teleport succeeded" : "Teleport failed"),
            "GTA5_Enhanced.Features");
    }
}
}
