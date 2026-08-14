#include "GTA_Self_Cache.hpp"

#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Manager.hpp"

namespace Devilz::Integrations::GTA5_Enhanced
{
GTA_Self_Cache& GTA_Self_Cache::Instance() noexcept
{
    static GTA_Self_Cache cache;
    return cache;
}

void GTA_Self_Cache::Reset() noexcept
{
    m_playerId.store(-1, std::memory_order_release);
    m_ped.store(0, std::memory_order_release);
    m_vehicle.store(0, std::memory_order_release);
    m_inVehicle.store(false, std::memory_order_release);
    m_revision.fetch_add(1, std::memory_order_acq_rel);
}

void GTA_Self_Cache::Update(GTA_Native_Manager& natives) noexcept
{
    if (!natives.Ready()) {
        Reset();
        return;
    }

    int playerId = -1;
    int ped = 0;
    int vehicle = 0;
    bool inVehicle = false;

    if (const auto player = natives.Invoke<int>(GTA_Native_Id::PlayerId))
        playerId = *player;

    if (const auto playerPed = natives.Invoke<int>(GTA_Native_Id::PlayerPedId))
        ped = *playerPed;

    if (ped != 0) {
        if (const auto seated = natives.Invoke<bool>(GTA_Native_Id::IsPedInAnyVehicle, ped, false))
            inVehicle = *seated;
        if (inVehicle) {
            if (const auto currentVehicle = natives.Invoke<int>(GTA_Native_Id::GetVehiclePedIsIn, ped, false))
                vehicle = *currentVehicle;
        }
    }

    m_playerId.store(playerId, std::memory_order_release);
    m_ped.store(ped, std::memory_order_release);
    m_vehicle.store(vehicle, std::memory_order_release);
    m_inVehicle.store(inVehicle && vehicle != 0, std::memory_order_release);
    m_revision.fetch_add(1, std::memory_order_acq_rel);
}
}
