#pragma once

#include "Integrations/GTA5_Enhanced/Types/Stats/GTA_Stat_Data.hpp"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace Devilz::Integrations::GTA5_Enhanced
{
class GTA_Native_Manager;

enum class GTA_Stat_Request_Status : std::uint8_t
{
    Idle,
    Queued,
    Succeeded,
    RuntimeUnavailable,
    NotFound,
    UnsupportedType,
    ControlledByNetShop,
    InvalidValue,
    QueueFull,
    Cancelled,
    Failed
};

struct GTA_Stat_Snapshot
{
    std::uint64_t revision = 0;
    std::uint64_t requestId = 0;
    GTA_Stat_Request_Status status = GTA_Stat_Request_Status::Idle;
    std::string requestedName;
    std::string normalizedName;
    std::uint32_t hash = 0;
    GTA_Stat_Data::Type type = GTA_Stat_Data::Type::Int;
    bool typeKnown = false;
    bool writeSupported = false;
    bool serverAuthoritative = false;
    bool controlledByNetShop = false;
    std::string value;
    std::string detail;
};

struct GTA_Stats_Queue_Snapshot
{
    std::uint64_t revision = 0;
    std::size_t pending = 0;
    std::size_t inFlight = 0;
    std::size_t capacity = 0;
    std::uint64_t completed = 0;
    std::uint64_t succeeded = 0;
    std::uint64_t failed = 0;
    std::uint64_t cancelled = 0;
    std::uint64_t rejected = 0;
};

class GTA_Stats_State final
{
public:
    static constexpr std::size_t MaxQueuedCommands = 512;
    static constexpr std::size_t MaxHistoryEntries = 128;

    static GTA_Stats_State& Instance() noexcept;

    GTA_Stats_State(const GTA_Stats_State&) = delete;
    GTA_Stats_State& operator=(const GTA_Stats_State&) = delete;

    [[nodiscard]] std::uint64_t RequestRead(std::string statName);
    [[nodiscard]] std::uint64_t RequestWrite(std::string statName, std::string value);
    [[nodiscard]] GTA_Stat_Snapshot Snapshot() const;
    [[nodiscard]] std::optional<GTA_Stat_Snapshot> Snapshot(std::uint64_t requestId) const;
    [[nodiscard]] GTA_Stats_Queue_Snapshot QueueSnapshot() const;
    [[nodiscard]] std::vector<GTA_Stat_Snapshot> RecentHistory(std::size_t maxCount = 32) const;
    [[nodiscard]] bool CancelPending(std::uint64_t requestId);
    [[nodiscard]] std::size_t ClearPending();
    void Reset();

    enum class Command_Kind : std::uint8_t
    {
        Read,
        Write
    };

    struct Command
    {
        std::uint64_t id = 0;
        Command_Kind kind = Command_Kind::Read;
        std::string statName;
        std::string value;
    };

private:
    GTA_Stats_State() = default;

    [[nodiscard]] std::uint64_t Queue(Command_Kind kind, std::string statName, std::string value);
    [[nodiscard]] bool Consume(Command& command);
    void Publish(GTA_Stat_Snapshot snapshot);
    void PushHistoryLocked(GTA_Stat_Snapshot snapshot);
    [[nodiscard]] GTA_Stat_Snapshot QueuedSnapshotLocked(const Command& command) const;

    friend std::size_t TickStatsExtension(GTA_Native_Manager& natives, std::size_t budget) noexcept;
    friend void ResetStatsExtension() noexcept;

    mutable std::mutex m_mutex;
    std::deque<Command> m_pending;
    std::optional<Command> m_inFlight;
    std::deque<GTA_Stat_Snapshot> m_history;
    GTA_Stat_Snapshot m_snapshot;
    std::uint64_t m_nextRequestId = 1;
    std::uint64_t m_revision = 0;
    std::uint64_t m_completed = 0;
    std::uint64_t m_succeeded = 0;
    std::uint64_t m_failed = 0;
    std::uint64_t m_cancelled = 0;
    std::uint64_t m_rejected = 0;
};

[[nodiscard]] const char* GTAStatTypeName(GTA_Stat_Data::Type type) noexcept;
[[nodiscard]] const char* GTAStatRequestStatusName(GTA_Stat_Request_Status status) noexcept;

std::size_t TickStatsExtension(GTA_Native_Manager& natives, std::size_t budget = 1) noexcept;
void ResetStatsExtension() noexcept;
}
