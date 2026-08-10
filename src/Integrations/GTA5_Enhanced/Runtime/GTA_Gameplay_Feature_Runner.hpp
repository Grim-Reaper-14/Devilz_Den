#pragma once

#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Call_Context.hpp"

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

    void TickGodMode() noexcept;
    void TickNeverWanted() noexcept;
    void TickTeleportToWaypoint() noexcept;
    void BeginTeleportToWaypoint() noexcept;
    void FinishTeleport(bool success, const char* detail) noexcept;

    GTA_Native_Manager* m_natives = nullptr;
    Backend::LoggerService* m_logger = nullptr;
    bool m_godModeApplied = false;
    bool m_neverWantedApplied = false;

    Teleport_Phase m_teleportPhase = Teleport_Phase::Idle;
    GTA_Native_Script_Vector m_waypoint{};
    std::uint32_t m_groundAttempts = 0;
};
}
