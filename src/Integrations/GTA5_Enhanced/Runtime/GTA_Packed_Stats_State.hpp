#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Devilz::Integrations::GTA5_Enhanced
{
enum class GTA_Packed_Stat_Batch_Kind : std::uint8_t
{
    None,
    Refresh,
    Unlock
};

struct GTA_Packed_Stat_Record
{
    bool known = false;
    bool value = false;
    bool queued = false;
    bool failed = false;
};

struct GTA_Packed_Stat_Batch_Snapshot
{
    std::uint64_t revision = 0;
    std::uint64_t batchId = 0;
    GTA_Packed_Stat_Batch_Kind kind = GTA_Packed_Stat_Batch_Kind::None;
    std::size_t total = 0;
    std::size_t completed = 0;
    std::size_t failed = 0;
    bool active = false;
};

class GTA_Packed_Stats_State final
{
public:
    static constexpr std::size_t MaxQueuedCommands = 2048;

    enum class Command_Kind : std::uint8_t
    {
        Read,
        Write
    };

    struct Command
    {
        std::uint64_t batchId = 0;
        Command_Kind kind = Command_Kind::Read;
        std::int32_t index = 0;
        bool value = false;
    };

    static GTA_Packed_Stats_State& Instance() noexcept
    {
        static GTA_Packed_Stats_State state;
        return state;
    }

    GTA_Packed_Stats_State(const GTA_Packed_Stats_State&) = delete;
    GTA_Packed_Stats_State& operator=(const GTA_Packed_Stats_State&) = delete;

    [[nodiscard]] std::uint64_t RequestReads(const std::vector<std::int32_t>& indices)
    {
        return QueueBatch(indices, GTA_Packed_Stat_Batch_Kind::Refresh, false);
    }

    [[nodiscard]] std::uint64_t RequestWrites(
        const std::vector<std::int32_t>& indices,
        bool value = true)
    {
        return QueueBatch(indices, GTA_Packed_Stat_Batch_Kind::Unlock, value);
    }

    [[nodiscard]] GTA_Packed_Stat_Record Snapshot(std::int32_t index) const
    {
        std::scoped_lock lock(m_mutex);
        const auto it = m_records.find(index);
        return it == m_records.end() ? GTA_Packed_Stat_Record{} : it->second;
    }

    [[nodiscard]] GTA_Packed_Stat_Batch_Snapshot BatchSnapshot() const
    {
        std::scoped_lock lock(m_mutex);
        return m_batch;
    }

    [[nodiscard]] bool Consume(Command& command)
    {
        std::scoped_lock lock(m_mutex);
        if (m_pending.empty())
            return false;
        command = m_pending.front();
        m_pending.pop_front();
        return true;
    }

    void Complete(const Command& command, bool success, bool value) noexcept
    {
        try {
            std::scoped_lock lock(m_mutex);
            auto& record = m_records[command.index];
            record.queued = false;
            record.failed = !success;
            if (success) {
                record.known = true;
                record.value = value;
            }

            if (command.batchId != m_batch.batchId)
                return;

            ++m_batch.completed;
            if (!success)
                ++m_batch.failed;
            if (m_batch.completed >= m_batch.total)
                m_batch.active = false;
            ++m_batch.revision;
        } catch (...) {
        }
    }

    void Reset() noexcept
    {
        try {
            std::scoped_lock lock(m_mutex);
            m_pending.clear();
            m_records.clear();
            m_batch = {};
            m_nextBatchId = 1;
        } catch (...) {
        }
    }

private:
    GTA_Packed_Stats_State() = default;

    void CancelQueuedLocked()
    {
        for (const auto& command : m_pending) {
            const auto it = m_records.find(command.index);
            if (it != m_records.end())
                it->second.queued = false;
        }
        m_pending.clear();
    }

    [[nodiscard]] std::uint64_t QueueBatch(
        const std::vector<std::int32_t>& indices,
        GTA_Packed_Stat_Batch_Kind kind,
        bool value)
    {
        std::scoped_lock lock(m_mutex);
        CancelQueuedLocked();

        GTA_Packed_Stat_Batch_Snapshot next{};
        next.batchId = m_nextBatchId++;
        next.kind = kind;
        next.revision = m_batch.revision + 1;

        std::unordered_set<std::int32_t> seen;
        seen.reserve(indices.size());
        const auto commandKind = kind == GTA_Packed_Stat_Batch_Kind::Refresh
            ? Command_Kind::Read
            : Command_Kind::Write;

        for (const auto index : indices) {
            if (index < 0 || !seen.insert(index).second)
                continue;
            if (m_pending.size() >= MaxQueuedCommands)
                break;

            m_pending.push_back(Command{next.batchId, commandKind, index, value});
            auto& record = m_records[index];
            record.queued = true;
            record.failed = false;
            ++next.total;
        }

        next.active = next.total != 0;
        m_batch = next;
        return next.batchId;
    }

    mutable std::mutex m_mutex;
    std::deque<Command> m_pending;
    std::unordered_map<std::int32_t, GTA_Packed_Stat_Record> m_records;
    GTA_Packed_Stat_Batch_Snapshot m_batch;
    std::uint64_t m_nextBatchId = 1;
};
}
