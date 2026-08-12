#include "GTA_Vehicle_Garage_Save.hpp"

#include "GTA_Vehicle_State.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Manager.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Registry.hpp"

#include <atomic>

namespace Devilz::Integrations::GTA5_Enhanced
{
namespace
{
std::atomic_bool g_lscPrepRequested{false};
std::atomic<GTA_Vehicle_LSC_Prep_Status> g_lscPrepStatus{GTA_Vehicle_LSC_Prep_Status::Idle};

[[nodiscard]] int CurrentVehicle(GTA_Native_Manager& natives) noexcept
{
    const auto ped = natives.Invoke<int>(GTA_Native_Id::PlayerPedId);
    if (!ped || *ped == 0)
        return 0;

    const auto seated = natives.Invoke<bool>(GTA_Native_Id::IsPedInAnyVehicle, *ped, false);
    if (!seated || !*seated)
        return 0;

    const auto vehicle = natives.Invoke<int>(GTA_Native_Id::GetVehiclePedIsIn, *ped, false);
    return vehicle && *vehicle != 0 ? *vehicle : 0;
}

[[nodiscard]] bool PrepareCurrentVehicleForLSC(GTA_Native_Manager& natives) noexcept
{
    const int vehicle = CurrentVehicle(natives);
    if (vehicle == 0) {
        g_lscPrepStatus.store(GTA_Vehicle_LSC_Prep_Status::NoVehicle, std::memory_order_release);
        return false;
    }

    const auto model = natives.Invoke<std::uint32_t>(GTA_Native_Id::GetEntityModel, vehicle);
    if (!model || *model == 0) {
        g_lscPrepStatus.store(GTA_Vehicle_LSC_Prep_Status::Failed, std::memory_order_release);
        return false;
    }

    bool success = true;
    success = natives.Invoke<void>(GTA_Native_Id::SetVehicleFixed, vehicle) && success;
    success = natives.Invoke<void>(GTA_Native_Id::SetVehicleDeformationFixed, vehicle) && success;
    success = natives.Invoke<void>(GTA_Native_Id::SetVehicleEngineHealth, vehicle, 1000.0F) && success;
    success = natives.Invoke<void>(GTA_Native_Id::SetVehicleBodyHealth, vehicle, 1000.0F) && success;
    success = natives.Invoke<void>(GTA_Native_Id::SetVehicleDirtLevel, vehicle, 0.0F) && success;
    success = natives.Invoke<void>(GTA_Native_Id::SetVehicleEngineOn, vehicle, true, true, false) && success;

    GTA_Vehicle_State::Instance().RequestForgeSnapshotRefresh();
    g_lscPrepStatus.store(
        success ? GTA_Vehicle_LSC_Prep_Status::Prepared : GTA_Vehicle_LSC_Prep_Status::Failed,
        std::memory_order_release);
    return success;
}
}

void RequestVehicleLSCPrep() noexcept
{
    g_lscPrepStatus.store(GTA_Vehicle_LSC_Prep_Status::Queued, std::memory_order_release);
    g_lscPrepRequested.store(true, std::memory_order_release);
}

GTA_Vehicle_LSC_Prep_Status VehicleLSCPrepStatus() noexcept
{
    return g_lscPrepStatus.load(std::memory_order_acquire);
}

void TickVehicleGarageSave(GTA_Native_Manager& natives) noexcept
{
    if (!g_lscPrepRequested.exchange(false, std::memory_order_acq_rel))
        return;

    g_lscPrepStatus.store(GTA_Vehicle_LSC_Prep_Status::Preparing, std::memory_order_release);
    (void)PrepareCurrentVehicleForLSC(natives);
}
}
