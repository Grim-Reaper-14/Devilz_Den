#pragma once

#include "GTA_Teleport_Locations.hpp"

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

    [[nodiscard]] bool MenuInputCaptured() const noexcept { return m_menuInputCaptured.load(std::memory_order_acquire); }
    void SetMenuInputCaptured(bool captured) noexcept { m_menuInputCaptured.store(captured, std::memory_order_release); }

    [[nodiscard]] bool GodMode() const noexcept { return m_godMode.load(std::memory_order_acquire); }
    void SetGodMode(bool enabled) noexcept { m_godMode.store(enabled, std::memory_order_release); }

    [[nodiscard]] bool NeverWanted() const noexcept { return m_neverWanted.load(std::memory_order_acquire); }
    void SetNeverWanted(bool enabled) noexcept { m_neverWanted.store(enabled, std::memory_order_release); }

    [[nodiscard]] bool SuperJump() const noexcept { return m_superJump.load(std::memory_order_acquire); }
    void SetSuperJump(bool enabled) noexcept { m_superJump.store(enabled, std::memory_order_release); }

    [[nodiscard]] bool InfiniteOxygen() const noexcept { return m_infiniteOxygen.load(std::memory_order_acquire); }
    void SetInfiniteOxygen(bool enabled) noexcept { m_infiniteOxygen.store(enabled, std::memory_order_release); }

    [[nodiscard]] bool NoRagdoll() const noexcept { return m_noRagdoll.load(std::memory_order_acquire); }
    void SetNoRagdoll(bool enabled) noexcept { m_noRagdoll.store(enabled, std::memory_order_release); }

    [[nodiscard]] bool KeepPlayerClean() const noexcept { return m_keepPlayerClean.load(std::memory_order_acquire); }
    void SetKeepPlayerClean(bool enabled) noexcept { m_keepPlayerClean.store(enabled, std::memory_order_release); }

    [[nodiscard]] bool InfiniteAmmo() const noexcept { return m_infiniteAmmo.load(std::memory_order_acquire); }
    void SetInfiniteAmmo(bool enabled) noexcept { m_infiniteAmmo.store(enabled, std::memory_order_release); }

    [[nodiscard]] bool UnlimitedClip() const noexcept { return m_unlimitedClip.load(std::memory_order_acquire); }
    void SetUnlimitedClip(bool enabled) noexcept { m_unlimitedClip.store(enabled, std::memory_order_release); }

    [[nodiscard]] bool ExplosiveBullets() const noexcept { return m_explosiveBullets.load(std::memory_order_acquire); }
    void SetExplosiveBullets(bool enabled) noexcept { m_explosiveBullets.store(enabled, std::memory_order_release); }

    [[nodiscard]] int ExplosionType() const noexcept { return m_explosionType.load(std::memory_order_acquire); }
    void SetExplosionType(int type) noexcept
    {
        if (type < -1) type = -1;
        if (type > 83) type = 83;
        m_explosionType.store(type, std::memory_order_release);
    }

    [[nodiscard]] float ExplosionDamageScale() const noexcept { return m_explosionDamageScale.load(std::memory_order_acquire); }
    void SetExplosionDamageScale(float scale) noexcept
    {
        if (scale < 0.0F) scale = 0.0F;
        if (scale > 1000.0F) scale = 1000.0F;
        m_explosionDamageScale.store(scale, std::memory_order_release);
    }

    [[nodiscard]] float ExplosionCameraShake() const noexcept { return m_explosionCameraShake.load(std::memory_order_acquire); }
    void SetExplosionCameraShake(float shake) noexcept
    {
        if (shake < 0.0F) shake = 0.0F;
        if (shake > 10.0F) shake = 10.0F;
        m_explosionCameraShake.store(shake, std::memory_order_release);
    }

    void RequestGiveAllWeapons() noexcept { m_giveAllWeaponsRequested.store(true, std::memory_order_release); }
    [[nodiscard]] bool ConsumeGiveAllWeaponsRequest() noexcept { return m_giveAllWeaponsRequested.exchange(false, std::memory_order_acq_rel); }

    void RequestGiveMaxAmmo() noexcept { m_giveMaxAmmoRequested.store(true, std::memory_order_release); }
    [[nodiscard]] bool ConsumeGiveMaxAmmoRequest() noexcept { return m_giveMaxAmmoRequested.exchange(false, std::memory_order_acq_rel); }

    void RequestRemoveAllWeapons() noexcept { m_removeAllWeaponsRequested.store(true, std::memory_order_release); }
    [[nodiscard]] bool ConsumeRemoveAllWeaponsRequest() noexcept { return m_removeAllWeaponsRequested.exchange(false, std::memory_order_acq_rel); }

    void RequestTeleportToWaypoint() noexcept
    {
        m_teleportStatus.store(GTA_Teleport_Waypoint_Status::Queued, std::memory_order_release);
        m_teleportWaypointRequested.store(true, std::memory_order_release);
    }

    [[nodiscard]] bool ConsumeTeleportToWaypointRequest() noexcept
    {
        return m_teleportWaypointRequested.exchange(false, std::memory_order_acq_rel);
    }

    void RequestTeleportToLocation(GTA_Teleport_Location_Id id) noexcept
    {
        m_requestedTeleportLocation.store(id, std::memory_order_release);
    }

    [[nodiscard]] GTA_Teleport_Location_Id ConsumeTeleportToLocationRequest() noexcept
    {
        return m_requestedTeleportLocation.exchange(GTA_Teleport_Location_Id::None, std::memory_order_acq_rel);
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
        m_menuInputCaptured.store(false, std::memory_order_release);
        m_godMode.store(false, std::memory_order_release);
        m_neverWanted.store(false, std::memory_order_release);
        m_superJump.store(false, std::memory_order_release);
        m_infiniteOxygen.store(false, std::memory_order_release);
        m_noRagdoll.store(false, std::memory_order_release);
        m_keepPlayerClean.store(false, std::memory_order_release);
        m_infiniteAmmo.store(false, std::memory_order_release);
        m_unlimitedClip.store(false, std::memory_order_release);
        m_explosiveBullets.store(false, std::memory_order_release);
        m_explosionType.store(45, std::memory_order_release);
        m_explosionDamageScale.store(1.0F, std::memory_order_release);
        m_explosionCameraShake.store(0.1F, std::memory_order_release);
        m_giveAllWeaponsRequested.store(false, std::memory_order_release);
        m_giveMaxAmmoRequested.store(false, std::memory_order_release);
        m_removeAllWeaponsRequested.store(false, std::memory_order_release);
        m_teleportWaypointRequested.store(false, std::memory_order_release);
        m_requestedTeleportLocation.store(GTA_Teleport_Location_Id::None, std::memory_order_release);
        m_teleportStatus.store(GTA_Teleport_Waypoint_Status::Idle, std::memory_order_release);
    }

private:
    GTA_Gameplay_State() = default;

    std::atomic_bool m_menuInputCaptured{false};
    std::atomic_bool m_godMode{false};
    std::atomic_bool m_neverWanted{false};
    std::atomic_bool m_superJump{false};
    std::atomic_bool m_infiniteOxygen{false};
    std::atomic_bool m_noRagdoll{false};
    std::atomic_bool m_keepPlayerClean{false};
    std::atomic_bool m_infiniteAmmo{false};
    std::atomic_bool m_unlimitedClip{false};
    std::atomic_bool m_explosiveBullets{false};
    std::atomic_int m_explosionType{45};
    std::atomic<float> m_explosionDamageScale{1.0F};
    std::atomic<float> m_explosionCameraShake{0.1F};
    std::atomic_bool m_giveAllWeaponsRequested{false};
    std::atomic_bool m_giveMaxAmmoRequested{false};
    std::atomic_bool m_removeAllWeaponsRequested{false};
    std::atomic_bool m_teleportWaypointRequested{false};
    std::atomic<GTA_Teleport_Location_Id> m_requestedTeleportLocation{GTA_Teleport_Location_Id::None};
    std::atomic<GTA_Teleport_Waypoint_Status> m_teleportStatus{GTA_Teleport_Waypoint_Status::Idle};
};
}
