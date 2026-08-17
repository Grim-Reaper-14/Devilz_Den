#pragma once

#include <atomic>
#include <cstdint>

namespace Devilz::Integrations::GTA5_Enhanced
{
class GTA_Native_Manager;

enum class GTA_Ped_Action : std::uint8_t
{
    None,
    KillEnemies,
    KillPeds
};

struct GTA_Ped_Action_Snapshot
{
    GTA_Ped_Action action = GTA_Ped_Action::None;
    std::uint32_t affected = 0;
    std::uint32_t skippedPlayers = 0;
    bool poolReady = false;
};

class GTA_Ped_Control_State final
{
public:
    static GTA_Ped_Control_State& Instance() noexcept;

    void SetRadius(float radius) noexcept;
    [[nodiscard]] float Radius() const noexcept;

    void RequestKillEnemies() noexcept;
    void RequestKillPeds() noexcept;
    [[nodiscard]] GTA_Ped_Action ConsumeAction() noexcept;

    void Publish(GTA_Ped_Action_Snapshot snapshot) noexcept;
    [[nodiscard]] GTA_Ped_Action_Snapshot Snapshot() const noexcept;
    void Reset() noexcept;

private:
    std::atomic<float> m_radius{150.0F};
    std::atomic<GTA_Ped_Action> m_action{GTA_Ped_Action::None};
    std::atomic<GTA_Ped_Action> m_lastAction{GTA_Ped_Action::None};
    std::atomic_uint32_t m_affected{0};
    std::atomic_uint32_t m_skippedPlayers{0};
    std::atomic_bool m_poolReady{false};
};

void TickPedControl(GTA_Native_Manager& natives) noexcept;
[[nodiscard]] bool PedControlHasWork() noexcept;
void ResetPedControl() noexcept;
}
