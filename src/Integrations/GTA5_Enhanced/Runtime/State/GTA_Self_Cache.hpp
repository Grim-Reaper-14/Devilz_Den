#pragma once

#include <atomic>
#include <cstdint>

namespace Devilz::Integrations::GTA5_Enhanced
{
class GTA_Native_Manager;

class GTA_Self_Cache final
{
public:
    static GTA_Self_Cache& Instance() noexcept;

    GTA_Self_Cache(const GTA_Self_Cache&) = delete;
    GTA_Self_Cache& operator=(const GTA_Self_Cache&) = delete;

    void Reset() noexcept;
    void Update(GTA_Native_Manager& natives) noexcept;

    [[nodiscard]] int PlayerId() const noexcept { return m_playerId.load(std::memory_order_acquire); }
    [[nodiscard]] int Ped() const noexcept { return m_ped.load(std::memory_order_acquire); }
    [[nodiscard]] int Vehicle() const noexcept { return m_vehicle.load(std::memory_order_acquire); }
    [[nodiscard]] bool InVehicle() const noexcept { return m_inVehicle.load(std::memory_order_acquire); }
    [[nodiscard]] std::uint64_t Revision() const noexcept { return m_revision.load(std::memory_order_acquire); }

private:
    GTA_Self_Cache() = default;

    std::atomic_int m_playerId{-1};
    std::atomic_int m_ped{0};
    std::atomic_int m_vehicle{0};
    std::atomic_bool m_inVehicle{false};
    std::atomic_uint64_t m_revision{0};
};
}
