#include "GTA_Vehicle_Tools.hpp"

#include "Backend/Logging/LoggerService.hpp"
#include "GTA_Vehicle_Saved_Builds.hpp"
#include "GTA_Vehicle_State.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Manager.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Registry.hpp"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace Devilz::Integrations::GTA5_Enhanced
{
namespace
{
// Canonical GET_CLOSEST_VEHICLE 0xF73EB622C4F1689B maps to this
// GTA V Enhanced runtime hash in YimMenuV2's Enhanced crossmap.
constexpr GTA_Native_Hash GetClosestVehicleEnhancedHash = 0xF0CA45A211FFDCD9ULL;
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
constexpr GTA_Native_Hash GetNeonEnabledHash = 0xF1B79038130E3C08ULL;
constexpr GTA_Native_Hash GetNeonColourHash = 0x64FEACF0AD019F1FULL;
constexpr GTA_Native_Hash GetXenonColourHash = 0xD6BA8C57BDF9DEB9ULL;
constexpr GTA_Native_Hash DoesExtraExistHash = 0x579FA5568DE0C2A0ULL;
constexpr GTA_Native_Hash IsExtraTurnedOnHash = 0x5318DF85BEB6B95FULL;
constexpr GTA_Native_Hash GetTyreSmokeColourHash = 0x9D35AABAEE206518ULL;
constexpr GTA_Native_Hash GetTyresCanBurstHash = 0xE6BE8A525BA6BD44ULL;
constexpr GTA_Native_Hash GetDriftTyresHash = 0x4497678941C27E46ULL;
constexpr GTA_Native_Hash GetInteriorColourHash = 0xE10BD9712D7B0CBFULL;
constexpr GTA_Native_Hash GetDashboardColourHash = 0x4C5611B5008205EBULL;
constexpr GTA_Native_Hash GetLiveryHash = 0xA089B04A208DBD0BULL;
constexpr GTA_Native_Hash GetLiveryCountHash = 0xBA3ECE95D3094B0FULL;

constexpr float CloneNearestRadius = 75.0F;
constexpr int CloneNearestFlags = 127;

Backend::LoggerService* g_logger = nullptr;
std::atomic_bool g_cloneRequested{false};
std::atomic_bool g_cloneAwaitingSpawn{false};
std::atomic<GTA_Vehicle_Clone_Nearest_Status> g_cloneStatus{GTA_Vehicle_Clone_Nearest_Status::Idle};

void LogVehicleTools(Backend::LogLevel level, std::string message) noexcept
{
    if (!g_logger)
        return;

    Backend::LogContext context;
    context.service = "GTA5_Enhanced.Vehicle.Tools";
    context.threadName = "GameThread";
    g_logger->LogWithContext(level, std::move(message), std::move(context));
}

[[nodiscard]] bool SpawnPipelineBusy(GTA_Vehicle_Spawn_Status status) noexcept
{
    return status == GTA_Vehicle_Spawn_Status::Queued ||
           status == GTA_Vehicle_Spawn_Status::Validating ||
           status == GTA_Vehicle_Spawn_Status::Streaming ||
           status == GTA_Vehicle_Spawn_Status::Creating ||
           status == GTA_Vehicle_Spawn_Status::Applying;
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

[[nodiscard]] std::string HexModel(std::uint32_t model)
{
    std::ostringstream stream;
    stream << "0x" << std::uppercase << std::hex << model;
    return stream.str();
}

[[nodiscard]] GTA_Vehicle_Metadata CaptureMetadata(
    GTA_Native_Manager& natives,
    std::uint32_t model)
{
    GTA_Vehicle_Metadata metadata{};
    metadata.modelHash = model;
    metadata.modelName = HexModel(model);
    metadata.displayName = metadata.modelName;

    const auto displayLabel = natives.Invoke<const char*>(GTA_Native_Id::GetDisplayNameFromVehicleModel, model);
    if (displayLabel && *displayLabel)
        metadata.displayName = CopyLocalized(natives, *displayLabel, metadata.modelName);

    const auto makeLabel = natives.Invoke<const char*>(GTA_Native_Id::GetMakeNameFromVehicleModel, model);
    if (makeLabel && *makeLabel)
        metadata.makeName = CopyLocalized(natives, *makeLabel, {});

    const auto vehicleClass = natives.Invoke<int>(GTA_Native_Id::GetVehicleClassFromName, model);
    if (vehicleClass)
        metadata.vehicleClass = *vehicleClass;
    return metadata;
}

[[nodiscard]] GTA_Vehicle_Forge_Snapshot CaptureSnapshot(
    GTA_Native_Manager& natives,
    int vehicle)
{
    GTA_Vehicle_Forge_Snapshot snapshot{};
    snapshot.vehicle = vehicle;

    const auto model = natives.Invoke<std::uint32_t>(GTA_Native_Id::GetEntityModel, vehicle);
    if (!model || *model == 0)
        return snapshot;
    snapshot.modelHash = *model;

    if (const auto wheelType = natives.Invoke<int>(GTA_Native_Id::GetVehicleWheelType, vehicle))
        snapshot.wheelType = *wheelType;
    if (const auto tint = natives.Invoke<int>(GTA_Native_Id::GetVehicleWindowTint, vehicle))
        snapshot.windowTint = *tint;
    if (const auto plateStyle = natives.Invoke<int>(GTA_Native_Id::GetVehicleNumberPlateTextIndex, vehicle))
        snapshot.plateStyle = *plateStyle;
    if (const auto plate = natives.InvokeHash<const char*>(GetPlateTextHash, vehicle); plate && *plate)
        snapshot.plateText = std::string(*plate).substr(0, 8);

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

    int pearl = primaryPearl;
    int wheelColor = -1;
    const bool gotExtraColours = natives.InvokeHash<void>(
        GetVehicleExtraColoursHash, vehicle, &pearl, &wheelColor);

    snapshot.paintStateReady = gotBaseColours || gotPrimary || gotSecondary || gotExtraColours;
    if (gotBaseColours) {
        snapshot.primaryColor = primaryBase;
        snapshot.secondaryColor = secondaryBase;
    } else {
        if (gotPrimary) snapshot.primaryColor = primaryColor;
        if (gotSecondary) snapshot.secondaryColor = secondaryColor;
    }
    if (gotPrimary) snapshot.primaryPaintType = primaryType;
    if (gotSecondary) snapshot.secondaryPaintType = secondaryType;
    if (gotExtraColours || primaryPearl >= 0) snapshot.pearlescentColor = pearl;
    if (gotExtraColours) snapshot.wheelColor = wheelColor;

    if (const auto custom = natives.InvokeHash<bool>(GetIsPrimaryCustomHash, vehicle)) {
        snapshot.primaryCustom = *custom;
        if (*custom) {
            int r = 0, g = 0, b = 0;
            if (natives.InvokeHash<void>(GetCustomPrimaryHash, vehicle, &r, &g, &b))
                snapshot.primaryRgb = {r, g, b};
        }
    }
    if (const auto custom = natives.InvokeHash<bool>(GetIsSecondaryCustomHash, vehicle)) {
        snapshot.secondaryCustom = *custom;
        if (*custom) {
            int r = 0, g = 0, b = 0;
            if (natives.InvokeHash<void>(GetCustomSecondaryHash, vehicle, &r, &g, &b))
                snapshot.secondaryRgb = {r, g, b};
        }
    }

    if (const auto turbo = natives.InvokeHash<bool>(IsToggleModOnHash, vehicle, 18))
        snapshot.turboEnabled = *turbo;
    if (const auto smoke = natives.InvokeHash<bool>(IsToggleModOnHash, vehicle, 20))
        snapshot.tireSmokeEnabled = *smoke;
    if (const auto xenon = natives.InvokeHash<bool>(IsToggleModOnHash, vehicle, 22))
        snapshot.xenonEnabled = *xenon;
    if (const auto xenonColor = natives.InvokeHash<int>(GetXenonColourHash, vehicle))
        snapshot.xenonColor = static_cast<std::int8_t>(*xenonColor);

    int neonR = 0, neonG = 0, neonB = 0;
    if (natives.InvokeHash<void>(GetNeonColourHash, vehicle, &neonR, &neonG, &neonB))
        snapshot.neonRgb = {neonR, neonG, neonB};
    for (int side = 0; side < 4; ++side) {
        if (const auto enabled = natives.InvokeHash<bool>(GetNeonEnabledHash, vehicle, side))
            snapshot.neonEnabled[static_cast<std::size_t>(side)] = *enabled;
    }

    int smokeR = 0, smokeG = 0, smokeB = 0;
    if (natives.InvokeHash<void>(GetTyreSmokeColourHash, vehicle, &smokeR, &smokeG, &smokeB))
        snapshot.tyreSmokeRgb = {smokeR, smokeG, smokeB};
    if (const auto front = natives.InvokeHash<bool>(GetVehicleModVariationHash, vehicle, 23))
        snapshot.frontCustomTires = *front;
    if (const auto rear = natives.InvokeHash<bool>(GetVehicleModVariationHash, vehicle, 24))
        snapshot.rearCustomTires = *rear;
    if (const auto canBurst = natives.InvokeHash<bool>(GetTyresCanBurstHash, vehicle))
        snapshot.tyresCanBurst = *canBurst;
    if (const auto drift = natives.InvokeHash<bool>(GetDriftTyresHash, vehicle))
        snapshot.driftTyres = *drift;

    for (int extra = 1; extra <= 14; ++extra) {
        const auto exists = natives.InvokeHash<bool>(DoesExtraExistHash, vehicle, extra);
        if (!exists || !*exists)
            continue;
        snapshot.extraExists[static_cast<std::size_t>(extra)] = true;
        if (const auto enabled = natives.InvokeHash<bool>(IsExtraTurnedOnHash, vehicle, extra))
            snapshot.extrasEnabled[static_cast<std::size_t>(extra)] = *enabled;
    }

    if (const auto liveryCount = natives.InvokeHash<int>(GetLiveryCountHash, vehicle))
        snapshot.liveryCount = (std::max)(*liveryCount, 0);
    if (const auto livery = natives.InvokeHash<int>(GetLiveryHash, vehicle))
        snapshot.livery = *livery;
    int interior = -1;
    if (natives.InvokeHash<void>(GetInteriorColourHash, vehicle, &interior))
        snapshot.interiorColor = interior;
    int dashboard = -1;
    if (natives.InvokeHash<void>(GetDashboardColourHash, vehicle, &dashboard))
        snapshot.dashboardColor = dashboard;

    for (int slot = 0; slot < 50; ++slot) {
        const auto count = natives.Invoke<int>(GTA_Native_Id::GetNumVehicleMods, vehicle, slot);
        if (!count || *count <= 0)
            continue;
        const auto installed = natives.Invoke<int>(GTA_Native_Id::GetVehicleMod, vehicle, slot);
        if (!installed)
            continue;
        GTA_Vehicle_Forge_Category category{};
        category.slot = slot;
        category.name = "Slot " + std::to_string(slot);
        category.installedIndex = *installed;
        snapshot.categories.push_back(std::move(category));
    }

    snapshot.modScanReady = true;
    return snapshot;
}

void UpdateCloneSpawnResult() noexcept
{
    if (!g_cloneAwaitingSpawn.load(std::memory_order_acquire))
        return;

    const auto spawnStatus = GTA_Vehicle_State::Instance().SpawnStatus();
    if (spawnStatus == GTA_Vehicle_Spawn_Status::Succeeded) {
        g_cloneAwaitingSpawn.store(false, std::memory_order_release);
        g_cloneStatus.store(GTA_Vehicle_Clone_Nearest_Status::Succeeded, std::memory_order_release);
        LogVehicleTools(Backend::LogLevel::Info, "Nearest vehicle clone spawned successfully");
    } else if (spawnStatus == GTA_Vehicle_Spawn_Status::Failed) {
        g_cloneAwaitingSpawn.store(false, std::memory_order_release);
        g_cloneStatus.store(GTA_Vehicle_Clone_Nearest_Status::Failed, std::memory_order_release);
        LogVehicleTools(Backend::LogLevel::Warning, "Nearest vehicle clone spawn failed");
    }
}
}

void ConfigureVehicleToolsLogging(Backend::LoggerService* logger) noexcept
{
    g_logger = logger;
}

void RequestCloneNearestVehicle() noexcept
{
    const auto status = g_cloneStatus.load(std::memory_order_acquire);
    if (status == GTA_Vehicle_Clone_Nearest_Status::Queued ||
        status == GTA_Vehicle_Clone_Nearest_Status::Searching ||
        status == GTA_Vehicle_Clone_Nearest_Status::SpawnQueued) {
        return;
    }

    g_cloneStatus.store(GTA_Vehicle_Clone_Nearest_Status::Queued, std::memory_order_release);
    g_cloneRequested.store(true, std::memory_order_release);
}

GTA_Vehicle_Clone_Nearest_Status CloneNearestVehicleStatus() noexcept
{
    return g_cloneStatus.load(std::memory_order_acquire);
}

void ResetVehicleTools() noexcept
{
    g_cloneRequested.store(false, std::memory_order_release);
    g_cloneAwaitingSpawn.store(false, std::memory_order_release);
    g_cloneStatus.store(GTA_Vehicle_Clone_Nearest_Status::Idle, std::memory_order_release);
    g_logger = nullptr;
}

void TickVehicleTools(GTA_Native_Manager& natives) noexcept
{
    UpdateCloneSpawnResult();

    if (!g_cloneRequested.exchange(false, std::memory_order_acq_rel))
        return;

    auto& state = GTA_Vehicle_State::Instance();
    if (SpawnPipelineBusy(state.SpawnStatus())) {
        g_cloneStatus.store(GTA_Vehicle_Clone_Nearest_Status::Failed, std::memory_order_release);
        LogVehicleTools(Backend::LogLevel::Warning,
            "Clone nearest vehicle rejected: vehicle spawn pipeline is busy");
        return;
    }

    g_cloneStatus.store(GTA_Vehicle_Clone_Nearest_Status::Searching, std::memory_order_release);

    const auto ped = natives.Invoke<int>(GTA_Native_Id::PlayerPedId);
    if (!ped || *ped == 0) {
        g_cloneStatus.store(GTA_Vehicle_Clone_Nearest_Status::Unavailable, std::memory_order_release);
        return;
    }

    const auto coords = natives.Invoke<GTA_Native_Script_Vector>(GTA_Native_Id::GetEntityCoords, *ped, true);
    if (!coords) {
        g_cloneStatus.store(GTA_Vehicle_Clone_Nearest_Status::Failed, std::memory_order_release);
        return;
    }

    const auto nearest = natives.InvokeOptionalHash<int>(
        GetClosestVehicleEnhancedHash,
        coords->x,
        coords->y,
        coords->z,
        CloneNearestRadius,
        0U,
        CloneNearestFlags);
    if (!nearest) {
        g_cloneStatus.store(GTA_Vehicle_Clone_Nearest_Status::Unavailable, std::memory_order_release);
        LogVehicleTools(Backend::LogLevel::Warning,
            "Clone nearest vehicle unavailable: optional GET_CLOSEST_VEHICLE handler was not resolved");
        return;
    }
    if (*nearest == 0) {
        g_cloneStatus.store(GTA_Vehicle_Clone_Nearest_Status::NoVehicle, std::memory_order_release);
        LogVehicleTools(Backend::LogLevel::Notice,
            "Clone nearest vehicle found no vehicle within 75 meters");
        return;
    }

    auto snapshot = CaptureSnapshot(natives, *nearest);
    if (snapshot.modelHash == 0) {
        g_cloneStatus.store(GTA_Vehicle_Clone_Nearest_Status::Failed, std::memory_order_release);
        LogVehicleTools(Backend::LogLevel::Warning,
            "Clone nearest vehicle failed while capturing the source vehicle");
        return;
    }

    auto metadata = CaptureMetadata(natives, snapshot.modelHash);
    auto build = GTA_Vehicle_Saved_Builds::Capture("Nearest Vehicle Clone", &metadata, snapshot);

    GTA_Vehicle_Spawn_Options options{};
    options.spawnInside = false;
    options.spawnMaxed = false;
    options.placeOnGround = true;
    options.engineRunning = true;
    options.invincible = state.VehicleGodMode();
    options.clean = true;

    state.SetPostSpawnForgeCommands(GTA_Vehicle_Saved_Builds::BuildCommands(build));
    state.RequestSpawn(snapshot.modelHash, options);
    g_cloneAwaitingSpawn.store(true, std::memory_order_release);
    g_cloneStatus.store(GTA_Vehicle_Clone_Nearest_Status::SpawnQueued, std::memory_order_release);

    LogVehicleTools(Backend::LogLevel::Info,
        "Nearest vehicle captured and queued for clone | Source: " + std::to_string(*nearest) +
            " | Model: " + HexModel(snapshot.modelHash) +
            " | Mods: " + std::to_string(build.mods.size()));
}
}
