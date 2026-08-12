#include "GTA_Vehicle_Garage_Save.hpp"

#include "GTA_Vehicle_State.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Manager.hpp"

#include <atomic>

namespace Devilz::Integrations::GTA5_Enhanced
{
namespace
{
std::atomic_bool g_garageSaveRequested{false};
std::atomic<GTA_Vehicle_Garage_Status> g_garageSaveStatus{GTA_Vehicle_Garage_Status::Idle};

[[nodiscard]] bool IsExistingPersonalVehicle(int vehicle) noexcept
{
    if (vehicle == 0)
        return false;

    // Fail closed until a validated legitimate personal-vehicle backend is
    // connected. This function must only return true for vehicles GTA already
    // considers owned by the local player; it must never create ownership.
    return false;
}

[[nodiscard]] bool PersistExistingPersonalVehicle(int vehicle) noexcept
{
    if (vehicle == 0)
        return false;

    // Placeholder for GTA's normal refresh/save path after legitimate
    // personal-vehicle validation. No ownership or garage slot is created here.
    return true;
}
}

void RequestVehicleGarageSave() noexcept
{
    g_garageSaveStatus.store(GTA_Vehicle_Garage_Status::Queued, std::memory_order_release);
    g_garageSaveRequested.store(true, std::memory_order_release);
}

GTA_Vehicle_Garage_Status VehicleGarageSaveStatus() noexcept
{
    return g_garageSaveStatus.load(std::memory_order_acquire);
}

GTA_Garage_Save_Result SaveCurrentForgeVehicleToGarage(GTA_Vehicle_State& state) noexcept
{
    const auto forge = state.ForgeSnapshot();

    if (forge.vehicle == 0)
        return GTA_Garage_Save_Result::NoVehicle;

    if (forge.modelHash == 0)
        return GTA_Garage_Save_Result::InvalidVehicle;

    if (!IsExistingPersonalVehicle(forge.vehicle))
        return GTA_Garage_Save_Result::OwnershipRegistrationRequired;

    state.RequestForgeSnapshotRefresh();

    if (!PersistExistingPersonalVehicle(forge.vehicle))
        return GTA_Garage_Save_Result::Failed;

    return GTA_Garage_Save_Result::Success;
}

void TickVehicleGarageSave(GTA_Native_Manager& natives) noexcept
{
    (void)natives;

    if (!g_garageSaveRequested.exchange(false, std::memory_order_acq_rel))
        return;

    g_garageSaveStatus.store(GTA_Vehicle_Garage_Status::Saving, std::memory_order_release);

    switch (SaveCurrentForgeVehicleToGarage(GTA_Vehicle_State::Instance())) {
    case GTA_Garage_Save_Result::Success:
        g_garageSaveStatus.store(GTA_Vehicle_Garage_Status::Saved, std::memory_order_release);
        break;
    case GTA_Garage_Save_Result::OwnershipRegistrationRequired:
    case GTA_Garage_Save_Result::NotOwned:
        g_garageSaveStatus.store(GTA_Vehicle_Garage_Status::OwnershipRequired, std::memory_order_release);
        break;
    default:
        g_garageSaveStatus.store(GTA_Vehicle_Garage_Status::Failed, std::memory_order_release);
        break;
    }
}
}
