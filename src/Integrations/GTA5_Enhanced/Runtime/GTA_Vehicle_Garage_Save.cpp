#include "GTA_Vehicle_Garage_Save.hpp"

#include "GTA_Vehicle_State.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Manager.hpp"

#include <atomic>

namespace Devilz::Integrations::GTA5_Enhanced
{
namespace
{
constexpr GTA_Native_Hash DoesEntityExistHash = 0xFC8BFE4B41177C22ULL;
constexpr GTA_Native_Hash IsEntityAVehicleHash = 0x55B80B6E7AB61270ULL;
constexpr GTA_Native_Hash DecorExistOnHash = 0xD130E7CDEE903624ULL;
constexpr GTA_Native_Hash DecorGetIntHash = 0xE2F6FE9B61232165ULL;

std::atomic_bool g_garageSaveRequested{false};
std::atomic<GTA_Vehicle_Garage_Status> g_garageSaveStatus{GTA_Vehicle_Garage_Status::Idle};

[[nodiscard]] bool IsExistingPersonalVehicle(GTA_Native_Manager& natives, int vehicle) noexcept
{
    if (vehicle == 0)
        return false;

    const auto exists = natives.InvokeHash<bool>(DoesEntityExistHash, vehicle);
    if (!exists || !*exists)
        return false;

    const auto isVehicle = natives.InvokeHash<bool>(IsEntityAVehicleHash, vehicle);
    if (!isVehicle || !*isVehicle)
        return false;

    const auto hasPlayerVehicleDecor =
        natives.InvokeHash<bool>(DecorExistOnHash, vehicle, "Player_Vehicle");
    if (!hasPlayerVehicleDecor || !*hasPlayerVehicleDecor)
        return false;

    const auto hasPvSlotDecor =
        natives.InvokeHash<bool>(DecorExistOnHash, vehicle, "PV_Slot");
    if (!hasPvSlotDecor || !*hasPvSlotDecor)
        return false;

    const auto pvSlot = natives.InvokeHash<int>(DecorGetIntHash, vehicle, "PV_Slot");
    if (!pvSlot || *pvSlot < 0)
        return false;

    // Require markers which GTA has already placed on an existing personal
    // vehicle. This validator never creates ownership or assigns a garage slot.
    return true;
}

[[nodiscard]] bool PersistExistingPersonalVehicle(
    GTA_Native_Manager& /*natives*/,
    int vehicle) noexcept
{
    if (vehicle == 0)
        return false;

    // Fail closed until GTA's legitimate existing-personal-vehicle refresh/save
    // path is connected. Returning true here would incorrectly report that a
    // vehicle was persisted when no save operation actually ran.
    return false;
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

GTA_Garage_Save_Result SaveCurrentForgeVehicleToGarage(
    GTA_Native_Manager& natives,
    GTA_Vehicle_State& state) noexcept
{
    const auto forge = state.ForgeSnapshot();

    if (forge.vehicle == 0)
        return GTA_Garage_Save_Result::NoVehicle;

    if (forge.modelHash == 0)
        return GTA_Garage_Save_Result::InvalidVehicle;

    if (!IsExistingPersonalVehicle(natives, forge.vehicle))
        return GTA_Garage_Save_Result::OwnershipRegistrationRequired;

    state.RequestForgeSnapshotRefresh();

    if (!PersistExistingPersonalVehicle(natives, forge.vehicle))
        return GTA_Garage_Save_Result::Failed;

    return GTA_Garage_Save_Result::Success;
}

void TickVehicleGarageSave(GTA_Native_Manager& natives) noexcept
{
    if (!g_garageSaveRequested.exchange(false, std::memory_order_acq_rel))
        return;

    g_garageSaveStatus.store(GTA_Vehicle_Garage_Status::Saving, std::memory_order_release);

    switch (SaveCurrentForgeVehicleToGarage(natives, GTA_Vehicle_State::Instance())) {
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
