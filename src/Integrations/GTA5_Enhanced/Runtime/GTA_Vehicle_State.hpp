#pragma once

#include <algorithm>
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
    CleanVehicle
};

struct GTA_Vehicle_Forge_Command
{
    GTA_Vehicle_Forge_Command_Type type = GTA_Vehicle_Forge_Command_Type::None;
    int arg0 = 0;
    int arg1 = 0;
    int arg2 = 0;
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

    [[nodiscard]] bool ConsumeSpawnRequest(
        std::uint32_t& modelHash,
        GTA_Vehicle_Spawn_Options& options) noexcept
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

    [[nodiscard]] GTA_Vehicle_Spawn_Status SpawnStatus() const noexcept
    {
        return m_spawnStatus.load(std::memory_order_acquire);
    }

    void SetSpawnStatus(GTA_Vehicle_Spawn_Status status) noexcept
    {
        m_spawnStatus.store(status, std::memory_order_release);
    }

    [[nodiscard]] int LastSpawnedVehicle() const noexcept
    {
        return m_lastSpawnedVehicle.load(std::memory_order_acquire);
    }

    void SetLastSpawnedVehicle(int vehicle) noexcept
    {
        m_lastSpawnedVehicle.store(vehicle, std::memory_order_release);
    }

    void QueueForgeCommand(const GTA_Vehicle_Forge_Command& command) noexcept
    {
        m_forgeArg0.store(command.arg0, std::memory_order_relaxed);
        m_forgeArg1.store(command.arg1, std::memory_order_relaxed);
        m_forgeArg2.store(command.arg2, std::memory_order_relaxed);
        m_forgeCommand.store(command.type, std::memory_order_release);
    }

    [[nodiscard]] GTA_Vehicle_Forge_Command ConsumeForgeCommand() noexcept
    {
        GTA_Vehicle_Forge_Command command{};
        command.type = m_forgeCommand.exchange(GTA_Vehicle_Forge_Command_Type::None, std::memory_order_acq_rel);
        command.arg0 = m_forgeArg0.load(std::memory_order_relaxed);
        command.arg1 = m_forgeArg1.load(std::memory_order_relaxed);
        command.arg2 = m_forgeArg2.load(std::memory_order_relaxed);
        return command;
    }

    void PublishMetadata(GTA_Vehicle_Metadata metadata)
    {
        std::scoped_lock lock(m_catalogMutex);
        m_catalog.push_back(std::move(metadata));
        m_catalogGeneration.fetch_add(1, std::memory_order_release);
    }

    [[nodiscard]] std::vector<GTA_Vehicle_Metadata> CatalogSnapshot() const
    {
        std::scoped_lock lock(m_catalogMutex);
        return m_catalog;
    }

    [[nodiscard]] std::uint64_t CatalogGeneration() const noexcept
    {
        return m_catalogGeneration.load(std::memory_order_acquire);
    }

    void Reset() noexcept
    {
        m_spawnRequested.store(false, std::memory_order_release);
        m_spawnModel.store(0, std::memory_order_release);
        m_spawnFlags.store(0, std::memory_order_release);
        m_spawnStatus.store(GTA_Vehicle_Spawn_Status::Idle, std::memory_order_release);
        m_lastSpawnedVehicle.store(0, std::memory_order_release);
        m_forgeCommand.store(GTA_Vehicle_Forge_Command_Type::None, std::memory_order_release);
    }

private:
    GTA_Vehicle_State() = default;

    std::atomic_bool m_spawnRequested{false};
    std::atomic<std::uint32_t> m_spawnModel{0};
    std::atomic<std::uint8_t> m_spawnFlags{0};
    std::atomic<GTA_Vehicle_Spawn_Status> m_spawnStatus{GTA_Vehicle_Spawn_Status::Idle};
    std::atomic_int m_lastSpawnedVehicle{0};
    std::atomic<GTA_Vehicle_Forge_Command_Type> m_forgeCommand{GTA_Vehicle_Forge_Command_Type::None};
    std::atomic_int m_forgeArg0{0};
    std::atomic_int m_forgeArg1{0};
    std::atomic_int m_forgeArg2{0};
    mutable std::mutex m_catalogMutex;
    std::vector<GTA_Vehicle_Metadata> m_catalog;
    std::atomic<std::uint64_t> m_catalogGeneration{0};
};
}
