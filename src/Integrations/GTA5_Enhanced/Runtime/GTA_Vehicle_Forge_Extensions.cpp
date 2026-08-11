#include "GTA_Vehicle_Forge_Extensions.hpp"

#include "GTA_Vehicle_State.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Manager.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Registry.hpp"

#include <array>
#include <cstddef>
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
constexpr std::size_t ExtensionCommandBatchSize = 8;

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
        std::string text = command.text.substr(0, 8);
        return natives.Invoke<void>(GTA_Native_Id::SetVehicleNumberPlateText, vehicle, text.c_str());
    }
    case GTA_Vehicle_Forge_Command_Type::ClearCustomPrimary:
        return natives.InvokeHash<void>(ClearCustomPrimaryHash, vehicle);
    case GTA_Vehicle_Forge_Command_Type::ClearCustomSecondary:
        return natives.InvokeHash<void>(ClearCustomSecondaryHash, vehicle);
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
    if (vehicle == 0 || snapshot.vehicle != vehicle)
        return;

    auto enriched = snapshot;

    int primaryBase = -1;
    int secondaryBase = -1;
    const bool gotBaseColours = natives.InvokeHash<void>(
        GetVehicleColoursHash, vehicle, &primaryBase, &secondaryBase);

    int primaryType = -1;
    int primaryColor = primaryBase;
    int primaryPearl = -1;
    const bool gotPrimary = natives.InvokeHash<void>(
        GetVehicleModColor1Hash, vehicle, &primaryType, &primaryColor, &primaryPearl);

    int secondaryType = -1;
    int secondaryColor = secondaryBase;
    const bool gotSecondary = natives.InvokeHash<void>(
        GetVehicleModColor2Hash, vehicle, &secondaryType, &secondaryColor);

    int pearlescent = primaryPearl;
    int wheelColor = -1;
    const bool gotExtras = natives.InvokeHash<void>(
        GetVehicleExtraColoursHash, vehicle, &pearlescent, &wheelColor);

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
    const auto xenon = natives.InvokeHash<bool>(IsToggleModOnHash, vehicle, 22);
    if (xenon)
        enriched.xenonEnabled = *xenon;
    const auto frontVariation = natives.InvokeHash<bool>(GetVehicleModVariationHash, vehicle, 23);
    if (frontVariation)
        enriched.frontCustomTires = *frontVariation;
    const auto rearVariation = natives.InvokeHash<bool>(GetVehicleModVariationHash, vehicle, 24);
    if (rearVariation)
        enriched.rearCustomTires = *rearVariation;

    const auto plateText = natives.InvokeHash<const char*>(GetPlateTextHash, vehicle);
    if (plateText && *plateText)
        enriched.plateText = std::string(*plateText).substr(0, 8);

    const bool changed = enriched.paintStateReady != snapshot.paintStateReady ||
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
              enriched.xenonEnabled != snapshot.xenonEnabled ||
              enriched.frontCustomTires != snapshot.frontCustomTires ||
              enriched.rearCustomTires != snapshot.rearCustomTires ||
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
