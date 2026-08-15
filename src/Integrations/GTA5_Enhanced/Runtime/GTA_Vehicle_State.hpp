#pragma once

#include "GTA_Vehicle_Catalog.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace Devilz::Integrations::GTA5_Enhanced
{
enum class GTA_Vehicle_Spawn_Status : std::uint8_t
{
    Idle,
    Queued,
    Validating,
    Streaming,
    Creating,
    Applying,
    Succeeded,
    Failed
};

struct GTA_Vehicle_Spawn_Options
{
    bool spawnInside = true;
    bool spawnMaxed = false;
    bool placeOnGround = true;
    bool engineRunning = true;
    bool invincible = false;
    bool clean = true;
};

enum class GTA_Vehicle_Forge_Command_Type : std::uint8_t
{
    None,
    SetMod,
    ToggleMod,
    SetWheelType,
    SetPrimaryPaint,
    SetSecondaryPaint,
    SetExtraColours,
    SetCustomPrimaryRgb,
    SetCustomSecondaryRgb,
    SetLoweredStance,
    SetWindowTint,
    SetPlateStyle,
    RepairVehicle,
    CleanVehicle,
    SetPlateText,
    ClearCustomPrimary,
    ClearCustomSecondary,
    SetNeonEnabled,
    SetNeonColor,
    SetXenonColor,
    SetExtra,
    SetTyreSmokeColor,
    SetTyresCanBurst,
    SetDriftTyres,
    SetInteriorColor,
    SetDashboardColor,
    SetLivery,
    SetEngineRunning,
    PlaceOnGround
};

struct GTA_Vehicle_Forge_Command
{
    GTA_Vehicle_Forge_Command_Type type = GTA_Vehicle_Forge_Command_Type::None;
    int arg0 = 0;
    int arg1 = 0;
    int arg2 = 0;
    std::string text;
};

struct GTA_Vehicle_Forge_Option
{
    int index = -1;
    std::string name;
};

struct GTA_Vehicle_Forge_Category
{
    int slot = -1;
    std::string name;
    int installedIndex = -1;
    std::vector<GTA_Vehicle_Forge_Option> options;
};

struct GTA_Vehicle_Forge_Snapshot
{
    int vehicle = 0;
    std::uint32_t modelHash = 0;
    int wheelType = -1;
    int windowTint = -1;
    int plateStyle = -1;
    std::string plateText;

    int modKitCount = -1;
    int modScanAttempts = 0;
    bool modScanReady = false;

    bool paintStateReady = false;
    int primaryPaintType = -1;
    int primaryColor = -1;
    int secondaryPaintType = -1;
    int secondaryColor = -1;
    int pearlescentColor = -1;
    int wheelColor = -1;
    bool primaryCustom = false;
    bool secondaryCustom = false;
    std::array<int, 3> primaryRgb{0, 0, 0};
    std::array<int, 3> secondaryRgb{0, 0, 0};

    bool turboEnabled = false;
    bool xenonEnabled = false;
    int xenonColor = -1;
    bool tireSmokeEnabled = false;
    std::array<int, 3> tyreSmokeRgb{255, 255, 255};
    bool frontCustomTires = false;
    bool rearCustomTires = false;
    bool tyresCanBurst = true;
    bool driftTyres = false;

    std::array<bool, 4> neonEnabled{false, false, false, false};
    std::array<int, 3> neonRgb{255, 255, 255};

    std::array<bool, 15> extraExists{};
    std::array<bool, 15> extrasEnabled{};
    int livery = -1;
    int liveryCount = 0;
    int interiorColor = -1;
    int dashboardColor = -1;

    std::vector<GTA_Vehicle_Forge_Category> categories;
};

struct GTA_Vehicle_Metadata
{
    std::uint32_t modelHash = 0;
    std::string modelName;
    std::string displayName;
    std::string makeName;
    int vehicleClass = -1;
};

class GTA_Vehicle_State final
{
public:
    static GTA_Vehicle_State& Instance() noexcept
    {
        static GTA_Vehicle_State state;
        return state;
    }

    GTA_Vehicle_State(const GTA_Vehicle_State&) = delete;
    GTA_Vehicle_State& operator=(const GTA_Vehicle_State&) = delete;

    void RequestSpawn(std::uint32_t modelHash, const GTA_Vehicle_Spawn_Options& options) noexcept
    {
        m_spawnModel.store(modelHash, std::memory_order_release);
        std::uint8_t flags = 0;
        if (options.spawnInside) flags |= 1U << 0U;
        if (options.spawnMaxed) flags |= 1U << 1U;
        if (options.placeOnGround) flags |= 1U << 2U;
        if (options.engineRunning) flags |= 1U << 3U;
        if (options.invincible) flags |= 1U << 4U;
        if (options.clean) flags |= 1U << 5U;
        m_spawnFlags.store(flags, std::memory_order_release);
        m_spawnStatus.store(GTA_Vehicle_Spawn_Status::Queued, std::memory_order_release);
        m_spawnRequested.store(true, std::memory_order_release);
    }

    [[nodiscard]] bool ConsumeSpawnRequest(std::uint32_t& modelHash, GTA_Vehicle_Spawn_Options& options) noexcept
    {
        if (!m_spawnRequested.exchange(false, std::memory_order_acq_rel))
            return false;
        modelHash = m_spawnModel.load(std::memory_order_acquire);
        const auto flags = m_spawnFlags.load(std::memory_order_acquire);
        options.spawnInside = (flags & (1U << 0U)) != 0;
        options.spawnMaxed = (flags & (1U << 1U)) != 0;
        options.placeOnGround = (flags & (1U << 2U)) != 0;
        options.engineRunning = (flags & (1U << 3U)) != 0;
        options.invincible = (flags & (1U << 4U)) != 0;
        options.clean = (flags & (1U << 5U)) != 0;
        return true;
    }

    [[nodiscard]] GTA_Vehicle_Spawn_Status SpawnStatus() const noexcept { return m_spawnStatus.load(std::memory_order_acquire); }
    void SetSpawnStatus(GTA_Vehicle_Spawn_Status status) noexcept { m_spawnStatus.store(status, std::memory_order_release); }
    [[nodiscard]] int LastSpawnedVehicle() const noexcept { return m_lastSpawnedVehicle.load(std::memory_order_acquire); }
    void SetLastSpawnedVehicle(int vehicle) noexcept { m_lastSpawnedVehicle.store(vehicle, std::memory_order_release); }

    void QueueForgeCommand(const GTA_Vehicle_Forge_Command& command)
    {
        if (command.type == GTA_Vehicle_Forge_Command_Type::None)
            return;
        {
            std::scoped_lock lock(m_forgeCommandMutex);
            m_forgeCommands.push_back(command);
        }
        RequestForgeSnapshotRefresh();
    }

    void QueueForgeCommands(const std::vector<GTA_Vehicle_Forge_Command>& commands)
    {
        if (commands.empty())
            return;
        {
            std::scoped_lock lock(m_forgeCommandMutex);
            for (const auto& command : commands)
                if (command.type != GTA_Vehicle_Forge_Command_Type::None)
                    m_forgeCommands.push_back(command);
        }
        RequestForgeSnapshotRefresh();
    }

    [[nodiscard]] GTA_Vehicle_Forge_Command ConsumeForgeCommand()
    {
        std::scoped_lock lock(m_forgeCommandMutex);
        const auto it = std::find_if(m_forgeCommands.begin(), m_forgeCommands.end(),
            [](const GTA_Vehicle_Forge_Command& command) {
                return command.type != GTA_Vehicle_Forge_Command_Type::None && !IsExtensionCommand(command.type);
            });
        if (it == m_forgeCommands.end())
            return {};
        const auto command = *it;
        m_forgeCommands.erase(it);
        return command;
    }

    [[nodiscard]] GTA_Vehicle_Forge_Command ConsumeForgeExtensionCommand()
    {
        std::scoped_lock lock(m_forgeCommandMutex);
        const auto it = std::find_if(m_forgeCommands.begin(), m_forgeCommands.end(),
            [](const GTA_Vehicle_Forge_Command& command) { return IsExtensionCommand(command.type); });
        if (it == m_forgeCommands.end())
            return {};
        const auto command = *it;
        m_forgeCommands.erase(it);
        return command;
    }

    void SetPostSpawnForgeCommands(std::vector<GTA_Vehicle_Forge_Command> commands)
    {
        std::scoped_lock lock(m_postSpawnCommandMutex);
        m_postSpawnForgeCommands.clear();
        m_postSpawnExtensionCommands.clear();
        for (auto& command : commands) {
            if (command.type == GTA_Vehicle_Forge_Command_Type::None)
                continue;
            if (IsExtensionCommand(command.type))
                m_postSpawnExtensionCommands.push_back(std::move(command));
            else
                m_postSpawnForgeCommands.push_back(std::move(command));
        }
    }

    [[nodiscard]] std::vector<GTA_Vehicle_Forge_Command> ConsumePostSpawnForgeCommands()
    {
        std::scoped_lock lock(m_postSpawnCommandMutex);
        auto commands = std::move(m_postSpawnForgeCommands);
        m_postSpawnForgeCommands.clear();
        return commands;
    }

    [[nodiscard]] std::vector<GTA_Vehicle_Forge_Command> ConsumePostSpawnExtensionCommands()
    {
        std::scoped_lock lock(m_postSpawnCommandMutex);
        auto commands = std::move(m_postSpawnExtensionCommands);
        m_postSpawnExtensionCommands.clear();
        return commands;
    }

    void SetKeepVehiclePerfect(bool enabled) noexcept { m_keepVehiclePerfect.store(enabled, std::memory_order_release); }
    [[nodiscard]] bool KeepVehiclePerfect() const noexcept { return m_keepVehiclePerfect.load(std::memory_order_acquire); }
    void SetVehicleGodMode(bool enabled) noexcept { m_vehicleGodMode.store(enabled, std::memory_order_release); }
    [[nodiscard]] bool VehicleGodMode() const noexcept { return m_vehicleGodMode.load(std::memory_order_acquire); }
    void RequestForgeSnapshotRefresh() noexcept { m_forgeSnapshotRefreshRequested.store(true, std::memory_order_release); }
    [[nodiscard]] bool ConsumeForgeSnapshotRefreshRequest() noexcept { return m_forgeSnapshotRefreshRequested.exchange(false, std::memory_order_acq_rel); }

    void PublishForgeSnapshot(GTA_Vehicle_Forge_Snapshot snapshot)
    {
        std::scoped_lock lock(m_forgeSnapshotMutex);
        m_forgeSnapshot = std::move(snapshot);
        m_forgeSnapshotGeneration.fetch_add(1, std::memory_order_release);
    }

    [[nodiscard]] GTA_Vehicle_Forge_Snapshot ForgeSnapshot() const
    {
        std::scoped_lock lock(m_forgeSnapshotMutex);
        return m_forgeSnapshot;
    }

    [[nodiscard]] std::uint64_t ForgeSnapshotGeneration() const noexcept { return m_forgeSnapshotGeneration.load(std::memory_order_acquire); }

    void PublishMetadata(GTA_Vehicle_Metadata metadata)
    {
        std::scoped_lock lock(m_catalogMutex);
        const auto existing = std::find_if(m_catalog.begin(), m_catalog.end(),
            [&metadata](const GTA_Vehicle_Metadata& entry) { return entry.modelHash == metadata.modelHash; });
        if (existing != m_catalog.end())
            *existing = std::move(metadata);
        else
            m_catalog.push_back(std::move(metadata));
        m_catalogGeneration.fetch_add(1, std::memory_order_release);
    }

    [[nodiscard]] std::vector<GTA_Vehicle_Metadata> CatalogSnapshot() const
    {
        std::scoped_lock lock(m_catalogMutex);
        return m_catalog;
    }

    [[nodiscard]] std::uint64_t CatalogGeneration() const noexcept { return m_catalogGeneration.load(std::memory_order_acquire); }

    void Reset() noexcept
    {
        m_spawnRequested.store(false, std::memory_order_release);
        m_spawnModel.store(0, std::memory_order_release);
        m_spawnFlags.store(0, std::memory_order_release);
        m_spawnStatus.store(GTA_Vehicle_Spawn_Status::Idle, std::memory_order_release);
        m_lastSpawnedVehicle.store(0, std::memory_order_release);
        {
            std::scoped_lock lock(m_forgeCommandMutex);
            m_forgeCommands.clear();
        }
        {
            std::scoped_lock lock(m_postSpawnCommandMutex);
            m_postSpawnForgeCommands.clear();
            m_postSpawnExtensionCommands.clear();
        }
        m_keepVehiclePerfect.store(false, std::memory_order_release);
        m_vehicleGodMode.store(false, std::memory_order_release);
        m_forgeSnapshotRefreshRequested.store(true, std::memory_order_release);
        {
            std::scoped_lock lock(m_forgeSnapshotMutex);
            m_forgeSnapshot = {};
        }
        m_forgeSnapshotGeneration.fetch_add(1, std::memory_order_release);
        {
            std::scoped_lock lock(m_catalogMutex);
            SeedStaticCatalogLocked();
        }
        m_catalogGeneration.fetch_add(1, std::memory_order_release);
    }

private:
    GTA_Vehicle_State() { SeedStaticCatalogLocked(); }

    [[nodiscard]] static bool IsExtensionCommand(GTA_Vehicle_Forge_Command_Type type) noexcept
    {
        switch (type) {
        case GTA_Vehicle_Forge_Command_Type::SetPrimaryPaint:
        case GTA_Vehicle_Forge_Command_Type::SetSecondaryPaint:
        case GTA_Vehicle_Forge_Command_Type::SetPlateText:
        case GTA_Vehicle_Forge_Command_Type::ClearCustomPrimary:
        case GTA_Vehicle_Forge_Command_Type::ClearCustomSecondary:
        case GTA_Vehicle_Forge_Command_Type::SetNeonEnabled:
        case GTA_Vehicle_Forge_Command_Type::SetNeonColor:
        case GTA_Vehicle_Forge_Command_Type::SetXenonColor:
        case GTA_Vehicle_Forge_Command_Type::SetExtra:
        case GTA_Vehicle_Forge_Command_Type::SetTyreSmokeColor:
        case GTA_Vehicle_Forge_Command_Type::SetTyresCanBurst:
        case GTA_Vehicle_Forge_Command_Type::SetDriftTyres:
        case GTA_Vehicle_Forge_Command_Type::SetInteriorColor:
        case GTA_Vehicle_Forge_Command_Type::SetDashboardColor:
        case GTA_Vehicle_Forge_Command_Type::SetLivery:
        case GTA_Vehicle_Forge_Command_Type::SetEngineRunning:
        case GTA_Vehicle_Forge_Command_Type::PlaceOnGround:
            return true;
        default:
            return false;
        }
    }

    void SeedStaticCatalogLocked()
    {
        m_catalog.clear();
        m_catalog.reserve(GTA_Vehicle_Model_Names.size());
        for (const auto modelName : GTA_Vehicle_Model_Names) {
            GTA_Vehicle_Metadata metadata{};
            metadata.modelHash = GTA_Model_Hash(modelName);
            metadata.modelName = std::string(modelName);
            metadata.displayName = metadata.modelName;
            m_catalog.push_back(std::move(metadata));
        }
    }

    std::atomic_bool m_spawnRequested{false};
    std::atomic<std::uint32_t> m_spawnModel{0};
    std::atomic<std::uint8_t> m_spawnFlags{0};
    std::atomic<GTA_Vehicle_Spawn_Status> m_spawnStatus{GTA_Vehicle_Spawn_Status::Idle};
    std::atomic_int m_lastSpawnedVehicle{0};
    mutable std::mutex m_forgeCommandMutex;
    std::vector<GTA_Vehicle_Forge_Command> m_forgeCommands;
    mutable std::mutex m_postSpawnCommandMutex;
    std::vector<GTA_Vehicle_Forge_Command> m_postSpawnForgeCommands;
    std::vector<GTA_Vehicle_Forge_Command> m_postSpawnExtensionCommands;
    std::atomic_bool m_keepVehiclePerfect{false};
    std::atomic_bool m_vehicleGodMode{false};
    std::atomic_bool m_forgeSnapshotRefreshRequested{true};
    mutable std::mutex m_forgeSnapshotMutex;
    GTA_Vehicle_Forge_Snapshot m_forgeSnapshot;
    std::atomic<std::uint64_t> m_forgeSnapshotGeneration{0};
    mutable std::mutex m_catalogMutex;
    std::vector<GTA_Vehicle_Metadata> m_catalog;
    std::atomic<std::uint64_t> m_catalogGeneration{0};
};
}
