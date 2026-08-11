#include "GTA_Vehicle_Forge_Extensions.hpp"

#include "GTA_Gameplay_State.hpp"
#include "GTA_Vehicle_State.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Manager.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Registry.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
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

constexpr GTA_Native_Hash SetRunSprintMultiplierHash = 0xA52E1AE3848A506BULL;
constexpr GTA_Native_Hash SetSwimMultiplierHash = 0x289497A4BA9049E0ULL;
constexpr GTA_Native_Hash SetPedMoveRateOverrideHash = 0xB27B08E34AC92345ULL;
constexpr GTA_Native_Hash IsPedArmedHash = 0x11552FA9DCB8E126ULL;
constexpr GTA_Native_Hash IsPedPerformingMeleeHash = 0xB73833BDAAE31047ULL;

constexpr std::size_t ExtensionCommandBatchSize = 16;
constexpr std::size_t CatalogResolveBatchSize = 16;
constexpr int MaxEmptyModScanAttempts = 12;
constexpr float FastRunMoveRateOverride = 2.0F;

int g_modScanVehicle = 0;
int g_emptyModScanAttempts = 0;
std::size_t g_catalogResolveCursor = 0;
void* g_scriptThread = nullptr;
int g_lastExplosionTimer = -1000;
GTA_Native_Script_Vector g_lastExplosionImpact{};

// GTA V Enhanced layout verified against YimMenuV2's scrThread/GtaThread
// definitions for this project generation: Context script hash @ 0x10,
// scrThread script hash @ 0x150, GtaThread script hash 2 @ 0x1A8.
struct GTA_Script_Thread_Spoof_View
{
    std::byte pad00[0x10]{};
    std::uint64_t contextScriptHash = 0;
    std::byte pad18[0x138]{};
    std::uint32_t scriptHash = 0;
    std::byte pad154[0x54]{};
    std::uint32_t scriptHash2 = 0;
};

static_assert(offsetof(GTA_Script_Thread_Spoof_View, contextScriptHash) == 0x10);
static_assert(offsetof(GTA_Script_Thread_Spoof_View, scriptHash) == 0x150);
static_assert(offsetof(GTA_Script_Thread_Spoof_View, scriptHash2) == 0x1A8);

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

[[nodiscard]] std::string CopyLocalized(
    GTA_Native_Manager& natives,
    const char* label,
    std::string fallback)
{
    if (!label || *label == '\0' || std::string_view(label) == "NULL")
        return fallback;
    const auto localized = natives.Invoke<const char*>(GTA_Native_Id::GetFilenameForAudioConversation, label);
    if (localized && *localized && **localized != '\0' && std::string_view(*localized) != "NULL")
        return std::string(*localized);
    return std::string(label);
}

void TickSelfMovement(GTA_Native_Manager& natives) noexcept
{
    auto& state = GTA_Gameplay_State::Instance();
    const auto player = natives.Invoke<int>(GTA_Native_Id::PlayerId);
    if (!player)
        return;

    const float runMultiplier = state.FastRun() ? state.RunSpeed() : 1.0F;
    const float swimMultiplier = state.FastSwim() ? state.SwimSpeed() : 1.0F;
    (void)natives.InvokeHash<void>(SetRunSprintMultiplierHash, *player, runMultiplier);
    (void)natives.InvokeHash<void>(SetSwimMultiplierHash, *player, swimMultiplier);

    if (state.FastRun()) {
        const auto ped = natives.Invoke<int>(GTA_Native_Id::PlayerPedId);
        if (ped && *ped != 0)
            (void)natives.InvokeHash<void>(SetPedMoveRateOverrideHash, *ped, FastRunMoveRateOverride);
    }
}

void TickExplosiveAmmo(GTA_Native_Manager& natives) noexcept
{
    auto& state = GTA_Gameplay_State::Instance();
    if (!state.ExplosiveBullets() || !g_scriptThread)
        return;

    const auto ped = natives.Invoke<int>(GTA_Native_Id::PlayerPedId);
    if (!ped || *ped == 0)
        return;

    const auto armed = natives.InvokeHash<bool>(IsPedArmedHash, *ped, 4);
    const auto melee = natives.InvokeHash<bool>(IsPedPerformingMeleeHash, *ped);
    if (!armed || !*armed || (melee && *melee))
        return;

    GTA_Native_Script_Vector impact{};
    const auto hit = natives.Invoke<bool>(GTA_Native_Id::GetPedLastWeaponImpactCoord, *ped, &impact);
    if (!hit || !*hit)
        return;

    const auto timer = natives.Invoke<int>(GTA_Native_Id::GetGameTimer);
    if (timer) {
        const bool sameImpact = std::fabs(impact.x - g_lastExplosionImpact.x) < 0.01F &&
            std::fabs(impact.y - g_lastExplosionImpact.y) < 0.01F &&
            std::fabs(impact.z - g_lastExplosionImpact.z) < 0.01F;
        if (sameImpact && *timer - g_lastExplosionTimer < 90)
            return;
        g_lastExplosionTimer = *timer;
        g_lastExplosionImpact = impact;
    }

    const auto orbitalHash = natives.Invoke<std::uint32_t>(GTA_Native_Id::GetHashKey, "am_mp_orbital_cannon");
    if (!orbitalHash)
        return;

    auto* thread = reinterpret_cast<GTA_Script_Thread_Spoof_View*>(g_scriptThread);
    const auto previousContextHash = thread->contextScriptHash;
    const auto previousHash = thread->scriptHash;
    const auto previousHash2 = thread->scriptHash2;

    thread->contextScriptHash = *orbitalHash;
    thread->scriptHash = *orbitalHash;
    thread->scriptHash2 = *orbitalHash;

    (void)natives.Invoke<void>(GTA_Native_Id::AddOwnedExplosion,
        *ped,
        impact.x,
        impact.y,
        impact.z,
        state.ExplosionType(),
        state.ExplosionDamageScale(),
        true,
        false,
        state.ExplosionCameraShake());

    thread->scriptHash2 = previousHash2;
    thread->scriptHash = previousHash;
    thread->contextScriptHash = previousContextHash;
}

void EnrichVehicleCatalog(GTA_Native_Manager& natives, GTA_Vehicle_State& state) noexcept
{
    auto catalog = state.CatalogSnapshot();
    if (catalog.empty())
        return;

    if (g_catalogResolveCursor >= catalog.size())
        g_catalogResolveCursor = 0;

    std::size_t visited = 0;
    std::size_t resolvedThisTick = 0;
    while (visited < catalog.size() && resolvedThisTick < CatalogResolveBatchSize) {
        const std::size_t index = g_catalogResolveCursor++ % catalog.size();
        ++visited;
        auto metadata = catalog[index];
        if (metadata.vehicleClass >= 0 && !metadata.makeName.empty() &&
            metadata.displayName != metadata.modelName)
            continue;

        const auto valid = natives.Invoke<bool>(GTA_Native_Id::IsModelInCdimage, metadata.modelHash);
        if (!valid || !*valid)
            continue;

        bool changed = false;
        if (metadata.vehicleClass < 0) {
            const auto vehicleClass = natives.Invoke<int>(GTA_Native_Id::GetVehicleClassFromName, metadata.modelHash);
            if (vehicleClass && *vehicleClass >= 0) {
                metadata.vehicleClass = *vehicleClass;
                changed = true;
            }
        }

        if (metadata.displayName.empty() || metadata.displayName == metadata.modelName) {
            const auto displayLabel = natives.Invoke<const char*>(GTA_Native_Id::GetDisplayNameFromVehicleModel, metadata.modelHash);
            if (displayLabel && *displayLabel) {
                metadata.displayName = CopyLocalized(natives, *displayLabel, metadata.modelName);
                changed = true;
            }
        }

        if (metadata.makeName.empty()) {
            const auto makeLabel = natives.Invoke<const char*>(GTA_Native_Id::GetMakeNameFromVehicleModel, metadata.modelHash);
            if (makeLabel && *makeLabel) {
                metadata.makeName = CopyLocalized(natives, *makeLabel, {});
                changed = true;
            }
        }

        if (changed) {
            state.PublishMetadata(std::move(metadata));
            ++resolvedThisTick;
        }
    }
}

bool ApplyExtensionCommand(
    GTA_Native_Manager& natives,
    int vehicle,
    const GTA_Vehicle_Forge_Command& command) noexcept
{
    if (vehicle == 0)
        return false;

    switch (command.type) {
    case GTA_Vehicle_Forge_Command_Type::SetPrimaryPaint: {
        int primary = 0;
        int secondary = 0;
        (void)natives.InvokeHash<void>(GetVehicleColoursHash, vehicle, &primary, &secondary);
        return natives.Invoke<void>(GTA_Native_Id::SetVehicleColours, vehicle, command.arg1, secondary);
    }
    case GTA_Vehicle_Forge_Command_Type::SetSecondaryPaint: {
        int primary = 0;
        int secondary = 0;
        (void)natives.InvokeHash<void>(GetVehicleColoursHash, vehicle, &primary, &secondary);
        return natives.Invoke<void>(GTA_Native_Id::SetVehicleColours, vehicle, primary, command.arg1);
    }
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
        (void)natives.Invoke<void>(GTA_Native_Id::SetVehicleModKit, vehicle, 0);
        (void)natives.Invoke<void>(GTA_Native_Id::ToggleVehicleMod, vehicle, 20, true);
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
        // SET_VEHICLE_COLOURS is authoritative for normal/chameleon palette
        // selection. Prefer those base indexes so the UI reflects what was
        // actually applied, while retaining mod-color type when GTA reports it.
        if (gotBaseColours) {
            enriched.primaryColor = primaryBase;
            enriched.secondaryColor = secondaryBase;
        } else {
            if (gotPrimary) enriched.primaryColor = primaryColor;
            if (gotSecondary) enriched.secondaryColor = secondaryColor;
        }
        if (gotPrimary) enriched.primaryPaintType = primaryType;
        if (gotSecondary) enriched.secondaryPaintType = secondaryType;
        if (gotExtras || primaryPearl >= 0) enriched.pearlescentColor = pearlescent;
        if (gotExtras) enriched.wheelColor = wheelColor;
    }

    const auto primaryCustom = natives.InvokeHash<bool>(GetIsPrimaryCustomHash, vehicle);
    if (primaryCustom) {
        enriched.primaryCustom = *primaryCustom;
        if (*primaryCustom) {
            int r = 0, g = 0, b = 0;
            if (natives.InvokeHash<void>(GetCustomPrimaryHash, vehicle, &r, &g, &b))
                enriched.primaryRgb = {r, g, b};
        }
    }

    const auto secondaryCustom = natives.InvokeHash<bool>(GetIsSecondaryCustomHash, vehicle);
    if (secondaryCustom) {
        enriched.secondaryCustom = *secondaryCustom;
        if (*secondaryCustom) {
            int r = 0, g = 0, b = 0;
            if (natives.InvokeHash<void>(GetCustomSecondaryHash, vehicle, &r, &g, &b))
                enriched.secondaryRgb = {r, g, b};
        }
    }

    const auto turbo = natives.InvokeHash<bool>(IsToggleModOnHash, vehicle, 18);
    if (turbo) enriched.turboEnabled = *turbo;
    const auto tireSmoke = natives.InvokeHash<bool>(IsToggleModOnHash, vehicle, 20);
    if (tireSmoke) enriched.tireSmokeEnabled = *tireSmoke;
    const auto xenon = natives.InvokeHash<bool>(IsToggleModOnHash, vehicle, 22);
    if (xenon) enriched.xenonEnabled = *xenon;

    const auto xenonColor = natives.InvokeHash<int>(GetXenonColourHash, vehicle);
    if (xenonColor) enriched.xenonColor = static_cast<std::int8_t>(*xenonColor);

    int neonR = 0, neonG = 0, neonB = 0;
    if (natives.InvokeHash<void>(GetNeonColourHash, vehicle, &neonR, &neonG, &neonB))
        enriched.neonRgb = {neonR, neonG, neonB};
    for (int side = 0; side < 4; ++side) {
        const auto enabled = natives.InvokeHash<bool>(GetNeonEnabledHash, vehicle, side);
        if (enabled) enriched.neonEnabled[static_cast<std::size_t>(side)] = *enabled;
    }

    int smokeR = 0, smokeG = 0, smokeB = 0;
    if (natives.InvokeHash<void>(GetTyreSmokeColourHash, vehicle, &smokeR, &smokeG, &smokeB))
        enriched.tyreSmokeRgb = {smokeR, smokeG, smokeB};

    const auto frontVariation = natives.InvokeHash<bool>(GetVehicleModVariationHash, vehicle, 23);
    if (frontVariation) enriched.frontCustomTires = *frontVariation;
    const auto rearVariation = natives.InvokeHash<bool>(GetVehicleModVariationHash, vehicle, 24);
    if (rearVariation) enriched.rearCustomTires = *rearVariation;
    const auto canBurst = natives.InvokeHash<bool>(GetTyresCanBurstHash, vehicle);
    if (canBurst) enriched.tyresCanBurst = *canBurst;
    const auto driftTyres = natives.InvokeHash<bool>(GetDriftTyresHash, vehicle);
    if (driftTyres) enriched.driftTyres = *driftTyres;

    for (int extra = 1; extra <= 14; ++extra) {
        const auto exists = natives.InvokeHash<bool>(DoesExtraExistHash, vehicle, extra);
        if (!exists)
            continue;
        enriched.extraExists[static_cast<std::size_t>(extra)] = *exists;
        if (*exists) {
            const auto enabled = natives.InvokeHash<bool>(IsExtraTurnedOnHash, vehicle, extra);
            if (enabled) enriched.extrasEnabled[static_cast<std::size_t>(extra)] = *enabled;
        }
    }

    const auto liveryCount = natives.InvokeHash<int>(GetLiveryCountHash, vehicle);
    if (liveryCount) enriched.liveryCount = (std::max)(*liveryCount, 0);
    const auto livery = natives.InvokeHash<int>(GetLiveryHash, vehicle);
    if (livery) enriched.livery = *livery;

    int interior = -1;
    if (natives.InvokeHash<void>(GetInteriorColourHash, vehicle, &interior)) enriched.interiorColor = interior;
    int dashboard = -1;
    if (natives.InvokeHash<void>(GetDashboardColourHash, vehicle, &dashboard)) enriched.dashboardColor = dashboard;

    const auto plateText = natives.InvokeHash<const char*>(GetPlateTextHash, vehicle);
    if (plateText && *plateText) enriched.plateText = std::string(*plateText).substr(0, 8);

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

void SetForgeExtensionScriptThread(void* scriptThread) noexcept
{
    g_scriptThread = scriptThread;
}

void TickVehicleForgeExtensions(GTA_Native_Manager& natives) noexcept
{
    auto& state = GTA_Vehicle_State::Instance();
    TickSelfMovement(natives);
    TickExplosiveAmmo(natives);
    EnrichVehicleCatalog(natives, state);
    DrainCurrentExtensionCommands(natives, state);
    DrainPostSpawnExtensionCommands(natives, state);
    EnrichForgeSnapshot(natives, state);
}
}
