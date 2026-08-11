#include "GTA_Vehicle_Forge_Extensions.hpp"

#include "GTA_Vehicle_State.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Manager.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Registry.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace Devilz::Integrations::GTA5_Enhanced
{
namespace
{
constexpr GTA_Native_Hash GetVehicleColoursHash = 0xFF4B16F297D9CB3EULL;
constexpr GTA_Native_Hash GetVehicleExtraColoursHash = 0x741D9B0685E67684ULL;
constexpr GTA_Native_Hash GetVehicleModColor1Hash = 0xB8090FC59766A88CULL;
constexpr GTA_Native_Hash GetVehicleModColor2Hash = 0x07AE5F5D5A7D0936ULL;
constexpr GTA_Native_Hash GetIsPrimaryCustomHash = 0xA9D64A14804D119BULL;
constexpr GTA_Native_Hash GetCustomPrimaryHash = 0xD9B9D4D1CCED7CA6ULL;
constexpr GTA_Native_Hash GetIsSecondaryCustomHash = 0x2C0B2BB7913E8DBAULL;
constexpr GTA_Native_Hash GetCustomSecondaryHash = 0x04434FA56DED5500ULL;
constexpr GTA_Native_Hash GetVehicleModVariationHash = 0xEFDD8C5443F6C9E4ULL;
constexpr GTA_Native_Hash IsToggleModOnHash = 0x1D5A665629D417A7ULL;
constexpr GTA_Native_Hash GetPlateTextHash = 0xCA7159F2C5FF745AULL;
constexpr GTA_Native_Hash ClearCustomPrimaryHash = 0x963D9A7202C06F65ULL;
constexpr GTA_Native_Hash ClearCustomSecondaryHash = 0x588D8FDC61F7CFADULL;
constexpr GTA_Native_Hash GetNumModKitsHash = 0x90E3EAFF8AAA1A42ULL;
constexpr GTA_Native_Hash SetNeonEnabledHash = 0xE62930EC6FAABCA5ULL;
constexpr GTA_Native_Hash GetNeonEnabledHash = 0xF1B79038130E3C08ULL;
constexpr GTA_Native_Hash SetNeonColourHash = 0xEAB8A43F6621850FULL;
constexpr GTA_Native_Hash GetNeonColourHash = 0x64FEACF0AD019F1FULL;
constexpr GTA_Native_Hash SetXenonColourHash = 0x89D1FDCA3735A1E0ULL;
constexpr GTA_Native_Hash GetXenonColourHash = 0xD6BA8C57BDF9DEB9ULL;
constexpr GTA_Native_Hash SetVehicleExtraHash = 0xD772F6AA66750D2BULL;
constexpr GTA_Native_Hash DoesExtraExistHash = 0x579FA5568DE0C2A0ULL;
constexpr GTA_Native_Hash IsExtraTurnedOnHash = 0x5318DF85BEB6B95FULL;
constexpr GTA_Native_Hash SetTyreSmokeColourHash = 0x5DA0536AEAD1FF31ULL;
constexpr GTA_Native_Hash GetTyreSmokeColourHash = 0x9D35AABAEE206518ULL;
constexpr GTA_Native_Hash SetTyresCanBurstHash = 0x439C904840715871ULL;
constexpr GTA_Native_Hash GetTyresCanBurstHash = 0xE6BE8A525BA6BD44ULL;
constexpr GTA_Native_Hash SetDriftTyresHash = 0x519F76A38952BBD0ULL;
constexpr GTA_Native_Hash GetDriftTyresHash = 0x4497678941C27E46ULL;
constexpr GTA_Native_Hash SetInteriorColourHash = 0xC0C8E6AAA00F1A58ULL;
constexpr GTA_Native_Hash GetInteriorColourHash = 0xE10BD9712D7B0CBFULL;
constexpr GTA_Native_Hash SetDashboardColourHash = 0x77B012A683295B6EULL;
constexpr GTA_Native_Hash GetDashboardColourHash = 0x4C5611B5008205EBULL;
constexpr GTA_Native_Hash SetLiveryHash = 0xA1C03303EC67320BULL;
constexpr GTA_Native_Hash GetLiveryHash = 0xA089B04A208DBD0BULL;
constexpr GTA_Native_Hash GetLiveryCountHash = 0xBA3ECE95D3094B0FULL;
constexpr std::size_t ExtensionCommandBatchSize = 16;
constexpr int MaxEmptyModScanAttempts = 12;

int g_modScanVehicle = 0;
int g_emptyModScanAttempts = 0;

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

bool ApplyExtensionCommand(
    GTA_Native_Manager& natives,
    int vehicle,
    const GTA_Vehicle_Forge_Command& command) noexcept
{
    if (vehicle == 0)
        return false;

    switch (command.type) {
    case GTA_Vehicle_Forge_Command_Type::SetPlateText: {
        const std::string text = command.text.substr(0, 8);
        return natives.Invoke<void>(GTA_Native_Id::SetVehicleNumberPlateText, vehicle, text.c_str());
    }
    case GTA_Vehicle_Forge_Command_Type::ClearCustomPrimary:
        return natives.InvokeHash<void>(ClearCustomPrimaryHash, vehicle);
    case GTA_Vehicle_Forge_Command_Type::ClearCustomSecondary:
        return natives.InvokeHash<void>(ClearCustomSecondaryHash, vehicle);
    case GTA_Vehicle_Forge_Command_Type::SetNeonEnabled:
        return natives.InvokeHash<void>(SetNeonEnabledHash, vehicle, std::clamp(command.arg0, 0, 3), command.arg1 != 0);
    case GTA_Vehicle_Forge_Command_Type::SetNeonColor:
        return natives.InvokeHash<void>(SetNeonColourHash, vehicle,
            std::clamp(command.arg0, 0, 255), std::clamp(command.arg1, 0, 255), std::clamp(command.arg2, 0, 255));
    case GTA_Vehicle_Forge_Command_Type::SetXenonColor:
        return natives.InvokeHash<void>(SetXenonColourHash, vehicle, command.arg0);
    case GTA_Vehicle_Forge_Command_Type::SetExtra:
        return natives.InvokeHash<void>(SetVehicleExtraHash, vehicle, command.arg0, command.arg1 == 0);
    case GTA_Vehicle_Forge_Command_Type::SetTyreSmokeColor:
        return natives.InvokeHash<void>(SetTyreSmokeColourHash, vehicle,
            std::clamp(command.arg0, 0, 255), std::clamp(command.arg1, 0, 255), std::clamp(command.arg2, 0, 255));
    case GTA_Vehicle_Forge_Command_Type::SetTyresCanBurst:
        return natives.InvokeHash<void>(SetTyresCanBurstHash, vehicle, command.arg0 != 0);
    case GTA_Vehicle_Forge_Command_Type::SetDriftTyres:
        return natives.InvokeHash<void>(SetDriftTyresHash, vehicle, command.arg0 != 0);
    case GTA_Vehicle_Forge_Command_Type::SetInteriorColor:
        return natives.InvokeHash<void>(SetInteriorColourHash, vehicle, command.arg0);
    case GTA_Vehicle_Forge_Command_Type::SetDashboardColor:
        return natives.InvokeHash<void>(SetDashboardColourHash, vehicle, command.arg0);
    case GTA_Vehicle_Forge_Command_Type::SetLivery:
        return natives.InvokeHash<void>(SetLiveryHash, vehicle, command.arg0);
    default:
        return false;
    }
}

void DrainCurrentExtensionCommands(GTA_Native_Manager& natives, GTA_Vehicle_State& state) noexcept
{
    for (std::size_t processed = 0; processed < ExtensionCommandBatchSize; ++processed) {
        const auto command = state.ConsumeForgeExtensionCommand();
        if (command.type == GTA_Vehicle_Forge_Command_Type::None)
            break;

        const int vehicle = CurrentVehicle(natives);
        if (vehicle != 0)
            (void)ApplyExtensionCommand(natives, vehicle, command);
    }
}

void DrainPostSpawnExtensionCommands(GTA_Native_Manager& natives, GTA_Vehicle_State& state) noexcept
{
    const auto status = state.SpawnStatus();
    if (status == GTA_Vehicle_Spawn_Status::Failed) {
        (void)state.ConsumePostSpawnExtensionCommands();
        return;
    }

    if (status != GTA_Vehicle_Spawn_Status::Succeeded)
        return;

    const int vehicle = state.LastSpawnedVehicle();
    if (vehicle == 0)
        return;

    for (const auto& command : state.ConsumePostSpawnExtensionCommands())
        (void)ApplyExtensionCommand(natives, vehicle, command);
}

void EnrichForgeSnapshot(GTA_Native_Manager& natives, GTA_Vehicle_State& state) noexcept
{
    const int vehicle = CurrentVehicle(natives);
    auto snapshot = state.ForgeSnapshot();
    if (vehicle == 0 || snapshot.vehicle != vehicle) {
        g_modScanVehicle = vehicle;
        g_emptyModScanAttempts = 0;
        return;
    }

    auto enriched = snapshot;

    if (vehicle != g_modScanVehicle) {
        g_modScanVehicle = vehicle;
        g_emptyModScanAttempts = 0;
    }

    if (snapshot.categories.empty()) {
        enriched.modScanReady = false;
        if (g_emptyModScanAttempts < MaxEmptyModScanAttempts) {
            ++g_emptyModScanAttempts;
            state.RequestForgeSnapshotRefresh();
        }
        enriched.modScanAttempts = g_emptyModScanAttempts;
    } else {
        g_emptyModScanAttempts = 0;
        enriched.modScanAttempts = 0;
        enriched.modScanReady = true;
    }

    const auto modKitCount = natives.InvokeHash<int>(GetNumModKitsHash, vehicle);
    if (modKitCount)
        enriched.modKitCount = *modKitCount;

    int primaryBase = -1;
    int secondaryBase = -1;
    const bool gotBaseColours = natives.InvokeHash<void>(GetVehicleColoursHash, vehicle, &primaryBase, &secondaryBase);

    int primaryType = -1;
    int primaryColor = primaryBase;
    int primaryPearl = -1;
    const bool gotPrimary = natives.InvokeHash<void>(GetVehicleModColor1Hash, vehicle, &primaryType, &primaryColor, &primaryPearl);

    int secondaryType = -1;
    int secondaryColor = secondaryBase;
    const bool gotSecondary = natives.InvokeHash<void>(GetVehicleModColor2Hash, vehicle, &secondaryType, &secondaryColor);

    int pearlescent = primaryPearl;
    int wheelColor = -1;
    const bool gotExtras = natives.InvokeHash<void>(GetVehicleExtraColoursHash, vehicle, &pearlescent, &wheelColor);

    if (gotPrimary || gotSecondary || gotBaseColours || gotExtras) {
        enriched.paintStateReady = true;
        if (gotPrimary) {
            enriched.primaryPaintType = primaryType;
            enriched.primaryColor = primaryColor;
        } else if (gotBaseColours) {
            enriched.primaryPaintType = 0;
            enriched.primaryColor = primaryBase;
        }

        if (gotSecondary) {
            enriched.secondaryPaintType = secondaryType;
            enriched.secondaryColor = secondaryColor;
        } else if (gotBaseColours) {
            enriched.secondaryPaintType = 0;
            enriched.secondaryColor = secondaryBase;
        }

        if (gotExtras || primaryPearl >= 0)
            enriched.pearlescentColor = pearlescent;
        if (gotExtras)
            enriched.wheelColor = wheelColor;
    }

    const auto primaryCustom = natives.InvokeHash<bool>(GetIsPrimaryCustomHash, vehicle);
    if (primaryCustom) {
        enriched.primaryCustom = *primaryCustom;
        if (*primaryCustom) {
            int r = 0;
            int g = 0;
            int b = 0;
            if (natives.InvokeHash<void>(GetCustomPrimaryHash, vehicle, &r, &g, &b))
                enriched.primaryRgb = {r, g, b};
        }
    }

    const auto secondaryCustom = natives.InvokeHash<bool>(GetIsSecondaryCustomHash, vehicle);
    if (secondaryCustom) {
        enriched.secondaryCustom = *secondaryCustom;
        if (*secondaryCustom) {
            int r = 0;
            int g = 0;
            int b = 0;
            if (natives.InvokeHash<void>(GetCustomSecondaryHash, vehicle, &r, &g, &b))
                enriched.secondaryRgb = {r, g, b};
        }
    }

    const auto turbo = natives.InvokeHash<bool>(IsToggleModOnHash, vehicle, 18);
    if (turbo)
        enriched.turboEnabled = *turbo;
    const auto tireSmoke = natives.InvokeHash<bool>(IsToggleModOnHash, vehicle, 20);
    if (tireSmoke)
        enriched.tireSmokeEnabled = *tireSmoke;
    const auto xenon = natives.InvokeHash<bool>(IsToggleModOnHash, vehicle, 22);
    if (xenon)
        enriched.xenonEnabled = *xenon;

    const auto xenonColor = natives.InvokeHash<int>(GetXenonColourHash, vehicle);
    if (xenonColor)
        enriched.xenonColor = static_cast<std::int8_t>(*xenonColor);

    int neonR = 0;
    int neonG = 0;
    int neonB = 0;
    if (natives.InvokeHash<void>(GetNeonColourHash, vehicle, &neonR, &neonG, &neonB))
        enriched.neonRgb = {neonR, neonG, neonB};
    for (int side = 0; side < 4; ++side) {
        const auto enabled = natives.InvokeHash<bool>(GetNeonEnabledHash, vehicle, side);
        if (enabled)
            enriched.neonEnabled[static_cast<std::size_t>(side)] = *enabled;
    }

    int smokeR = 0;
    int smokeG = 0;
    int smokeB = 0;
    if (natives.InvokeHash<void>(GetTyreSmokeColourHash, vehicle, &smokeR, &smokeG, &smokeB))
        enriched.tyreSmokeRgb = {smokeR, smokeG, smokeB};

    const auto frontVariation = natives.InvokeHash<bool>(GetVehicleModVariationHash, vehicle, 23);
    if (frontVariation)
        enriched.frontCustomTires = *frontVariation;
    const auto rearVariation = natives.InvokeHash<bool>(GetVehicleModVariationHash, vehicle, 24);
    if (rearVariation)
        enriched.rearCustomTires = *rearVariation;
    const auto canBurst = natives.InvokeHash<bool>(GetTyresCanBurstHash, vehicle);
    if (canBurst)
        enriched.tyresCanBurst = *canBurst;
    const auto driftTyres = natives.InvokeHash<bool>(GetDriftTyresHash, vehicle);
    if (driftTyres)
        enriched.driftTyres = *driftTyres;

    for (int extra = 1; extra <= 14; ++extra) {
        const auto exists = natives.InvokeHash<bool>(DoesExtraExistHash, vehicle, extra);
        if (!exists)
            continue;
        enriched.extraExists[static_cast<std::size_t>(extra)] = *exists;
        if (*exists) {
            const auto enabled = natives.InvokeHash<bool>(IsExtraTurnedOnHash, vehicle, extra);
            if (enabled)
                enriched.extrasEnabled[static_cast<std::size_t>(extra)] = *enabled;
        }
    }

    const auto liveryCount = natives.InvokeHash<int>(GetLiveryCountHash, vehicle);
    if (liveryCount)
        enriched.liveryCount = (std::max)(*liveryCount, 0);
    const auto livery = natives.InvokeHash<int>(GetLiveryHash, vehicle);
    if (livery)
        enriched.livery = *livery;

    int interior = -1;
    if (natives.InvokeHash<void>(GetInteriorColourHash, vehicle, &interior))
        enriched.interiorColor = interior;
    int dashboard = -1;
    if (natives.InvokeHash<void>(GetDashboardColourHash, vehicle, &dashboard))
        enriched.dashboardColor = dashboard;

    const auto plateText = natives.InvokeHash<const char*>(GetPlateTextHash, vehicle);
    if (plateText && *plateText)
        enriched.plateText = std::string(*plateText).substr(0, 8);

    const bool changed = enriched.modKitCount != snapshot.modKitCount ||
        enriched.modScanAttempts != snapshot.modScanAttempts ||
        enriched.modScanReady != snapshot.modScanReady ||
        enriched.paintStateReady != snapshot.paintStateReady ||
        enriched.primaryPaintType != snapshot.primaryPaintType ||
        enriched.primaryColor != snapshot.primaryColor ||
        enriched.secondaryPaintType != snapshot.secondaryPaintType ||
        enriched.secondaryColor != snapshot.secondaryColor ||
        enriched.pearlescentColor != snapshot.pearlescentColor ||
        enriched.wheelColor != snapshot.wheelColor ||
        enriched.primaryCustom != snapshot.primaryCustom ||
        enriched.secondaryCustom != snapshot.secondaryCustom ||
        enriched.primaryRgb != snapshot.primaryRgb ||
        enriched.secondaryRgb != snapshot.secondaryRgb ||
        enriched.turboEnabled != snapshot.turboEnabled ||
        enriched.tireSmokeEnabled != snapshot.tireSmokeEnabled ||
        enriched.xenonEnabled != snapshot.xenonEnabled ||
        enriched.xenonColor != snapshot.xenonColor ||
        enriched.neonEnabled != snapshot.neonEnabled ||
        enriched.neonRgb != snapshot.neonRgb ||
        enriched.tyreSmokeRgb != snapshot.tyreSmokeRgb ||
        enriched.frontCustomTires != snapshot.frontCustomTires ||
        enriched.rearCustomTires != snapshot.rearCustomTires ||
        enriched.tyresCanBurst != snapshot.tyresCanBurst ||
        enriched.driftTyres != snapshot.driftTyres ||
        enriched.extraExists != snapshot.extraExists ||
        enriched.extrasEnabled != snapshot.extrasEnabled ||
        enriched.livery != snapshot.livery ||
        enriched.liveryCount != snapshot.liveryCount ||
        enriched.interiorColor != snapshot.interiorColor ||
        enriched.dashboardColor != snapshot.dashboardColor ||
        enriched.plateText != snapshot.plateText;

    if (changed)
        state.PublishForgeSnapshot(std::move(enriched));
}
}

void TickVehicleForgeExtensions(GTA_Native_Manager& natives) noexcept
{
    auto& state = GTA_Vehicle_State::Instance();
    DrainCurrentExtensionCommands(natives, state);
    DrainPostSpawnExtensionCommands(natives, state);
    EnrichForgeSnapshot(natives, state);
}
}
