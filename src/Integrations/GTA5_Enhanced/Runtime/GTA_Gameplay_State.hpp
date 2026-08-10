#pragma once

#include <atomic>
#include <cstdint>

namespace Devilz::Integrations::GTA5_Enhanced
{
enum class GTA_Teleport_Waypoint_Status : std::uint8_t
{
    Idle,
    Queued,
    Resolving,
    Succeeded,
    NoWaypoint,
    Failed
};

class GTA_Gameplay_State final
{
public:
    static GTA_Gameplay_State& Instance() noexcept
    {
        static GTA_Gameplay_State state;
        return state;
    }

    GTA_Gameplay_State(const GTA_Gameplay_State&) = delete;
    GTA_Gameplay_State& operator=(const GTA_Gameplay_State&) = delete;

    [[nodiscard]] bool GodMode() const noexcept
    {
        return m_godMode.load(std::memory_order_acquire);
    }

    void SetGodMode(bool enabled) noexcept
    {
        m_godMode.store(enabled, std::memory_order_release);
    }

    [[nodiscard]] bool NeverWanted() const noexcept
    {
        return m_neverWanted.load(std::memory_order_acquire);
    }

    void SetNeverWanted(bool enabled) noexcept
    {
        m_neverWanted.store(enabled, std::memory_order_release);
    }

    void RequestTeleportToWaypoint() noexcept
    {
        m_teleportStatus.store(GTA_Teleport_Waypoint_Status::Queued, std::memory_order_release);
        m_teleportWaypointRequested.store(true, std::memory_order_release);
    }

    [[nodiscard]] bool ConsumeTeleportToWaypointRequest() noexcept
    {
        return m_teleportWaypointRequested.exchange(false, std::memory_order_acq_rel);
    }

    [[nodiscard]] GTA_Teleport_Waypoint_Status TeleportStatus() const noexcept
    {
        return m_teleportStatus.load(std::memory_order_acquire);
    }

    void SetTeleportStatus(GTA_Teleport_Waypoint_Status status) noexcept
    {
        m_teleportStatus.store(status, std::memory_order_release);
    }

    void Reset() noexcept
    {
        m_godMode.store(false, std::memory_order_release);
        m_neverWanted.store(false, std::memory_order_release);
        m_teleportWaypointRequested.store(false, std::memory_order_release);
        m_teleportStatus.store(GTA_Teleport_Waypoint_Status::Idle, std::memory_order_release);
    }

private:
    GTA_Gameplay_State() = default;

    std::atomic_bool m_godMode{false};
    std::atomic_bool m_neverWanted{false};
    std::atomic_bool m_teleportWaypointRequested{false};
    std::atomic<GTA_Teleport_Waypoint_Status> m_teleportStatus{GTA_Teleport_Waypoint_Status::Idle};
};
}
