#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace Devilz::Integrations::GTA5_Enhanced
{
enum class GTA_Unlock_Operation_Type : std::uint8_t
{
    PackedBool,
    StatBool,
    StatInt,
    TunableInt
};

struct GTA_Unlock_Operation
{
    GTA_Unlock_Operation_Type type = GTA_Unlock_Operation_Type::PackedBool;
    std::int32_t index = -1;
    std::string statName;
    std::int32_t intValue = 0;
    bool boolValue = true;

    [[nodiscard]] static GTA_Unlock_Operation PackedBool(std::int32_t packedIndex, bool value = true)
    {
        return {GTA_Unlock_Operation_Type::PackedBool, packedIndex, {}, 0, value};
    }

    [[nodiscard]] static GTA_Unlock_Operation StatBool(std::string name, bool value = true)
    {
        return {GTA_Unlock_Operation_Type::StatBool, -1, std::move(name), 0, value};
    }

    [[nodiscard]] static GTA_Unlock_Operation StatInt(std::string name, std::int32_t value)
    {
        return {GTA_Unlock_Operation_Type::StatInt, -1, std::move(name), value, false};
    }

    // globalIndex is an absolute GTA script-global index, e.g. 262145 + a verified tunable offset.
    [[nodiscard]] static GTA_Unlock_Operation TunableInt(std::int32_t globalIndex, std::int32_t value)
    {
        return {GTA_Unlock_Operation_Type::TunableInt, globalIndex, {}, value, false};
    }
};

enum class GTA_Unlock_Batch_Kind : std::uint8_t
{
    None,
    Refresh,
    Apply
};

struct GTA_Unlock_Operation_Record
{
    bool known = false;
    bool matched = false;
    bool queued = false;
    bool failed = false;
    std::string detail;
};

struct GTA_Unlock_Batch_Snapshot
{
    std::uint64_t revision = 0;
    std::uint64_t batchId = 0;
    GTA_Unlock_Batch_Kind kind = GTA_Unlock_Batch_Kind::None;
    std::size_t total = 0;
    std::size_t completed = 0;
    std::size_t failed = 0;
    bool active = false;
};

class GTA_Unlock_Operations_State final
{
public:
    static constexpr std::size_t MaxQueuedCommands = 4096;

    enum class Command_Kind : std::uint8_t
    {
        Read,
        Write
    };

    struct Command
    {
        std::uint64_t batchId = 0;
        Command_Kind kind = Command_Kind::Read;
        GTA_Unlock_Operation operation{};
        std::string key;
    };

    static GTA_Unlock_Operations_State& Instance() noexcept
    {
        static GTA_Unlock_Operations_State state;
        return state;
    }

    GTA_Unlock_Operations_State(const GTA_Unlock_Operations_State&) = delete;
    GTA_Unlock_Operations_State& operator=(const GTA_Unlock_Operations_State&) = delete;

    [[nodiscard]] std::uint64_t RequestRefresh(const std::vector<GTA_Unlock_Operation>& operations)
    {
        return QueueBatch(operations, GTA_Unlock_Batch_Kind::Refresh);
    }

    [[nodiscard]] std::uint64_t RequestApply(const std::vector<GTA_Unlock_Operation>& operations)
    {
        return QueueBatch(operations, GTA_Unlock_Batch_Kind::Apply);
    }

    [[nodiscard]] GTA_Unlock_Operation_Record Snapshot(const GTA_Unlock_Operation& operation) const
    {
        const auto key = Key(operation);
        std::scoped_lock lock(m_mutex);
        const auto it = m_records.find(key);
        return it == m_records.end() ? GTA_Unlock_Operation_Record{} : it->second;
    }

    [[nodiscard]] GTA_Unlock_Batch_Snapshot BatchSnapshot() const
    {
        std::scoped_lock lock(m_mutex);
        return m_batch;
    }

    [[nodiscard]] bool Consume(Command& command)
    {
        std::scoped_lock lock(m_mutex);
        if (m_pending.empty())
            return false;
        command = std::move(m_pending.front());
        m_pending.pop_front();
        return true;
    }

    void Complete(
        const Command& command,
        bool success,
        bool known,
        bool matched,
        std::string detail = {}) noexcept
    {
        try {
            std::scoped_lock lock(m_mutex);
            auto& record = m_records[command.key];
            record.queued = false;
            record.failed = !success;
            record.detail = std::move(detail);
            if (success) {
                record.known = known;
                record.matched = matched;
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

    [[nodiscard]] static std::string Key(const GTA_Unlock_Operation& operation)
    {
        const char prefix = operation.type == GTA_Unlock_Operation_Type::PackedBool ? 'P' :
            operation.type == GTA_Unlock_Operation_Type::StatBool ? 'B' :
            operation.type == GTA_Unlock_Operation_Type::StatInt ? 'I' : 'T';
        if (operation.type == GTA_Unlock_Operation_Type::StatBool ||
            operation.type == GTA_Unlock_Operation_Type::StatInt) {
            return std::string(1, prefix) + ':' + operation.statName;
        }
        return std::string(1, prefix) + ':' + std::to_string(operation.index);
    }

private:
    GTA_Unlock_Operations_State() = default;

    void CancelQueuedLocked()
    {
        for (const auto& command : m_pending) {
            const auto it = m_records.find(command.key);
            if (it != m_records.end())
                it->second.queued = false;
        }
        m_pending.clear();
    }

    [[nodiscard]] std::uint64_t QueueBatch(
        const std::vector<GTA_Unlock_Operation>& operations,
        GTA_Unlock_Batch_Kind kind)
    {
        std::scoped_lock lock(m_mutex);
        CancelQueuedLocked();

        GTA_Unlock_Batch_Snapshot next{};
        next.batchId = m_nextBatchId++;
        next.kind = kind;
        next.revision = m_batch.revision + 1;

        std::unordered_set<std::string> seen;
        seen.reserve(operations.size());
        const auto commandKind = kind == GTA_Unlock_Batch_Kind::Refresh
            ? Command_Kind::Read
            : Command_Kind::Write;

        for (const auto& operation : operations) {
            if ((operation.type == GTA_Unlock_Operation_Type::PackedBool ||
                 operation.type == GTA_Unlock_Operation_Type::TunableInt) && operation.index < 0) {
                continue;
            }
            if ((operation.type == GTA_Unlock_Operation_Type::StatBool ||
                 operation.type == GTA_Unlock_Operation_Type::StatInt) && operation.statName.empty()) {
                continue;
            }
            if (m_pending.size() >= MaxQueuedCommands)
                break;

            const auto key = Key(operation);
            if (!seen.insert(key).second)
                continue;

            m_pending.push_back(Command{next.batchId, commandKind, operation, key});
            auto& record = m_records[key];
            record.queued = true;
            record.failed = false;
            record.detail.clear();
            ++next.total;
        }

        next.active = next.total != 0;
        m_batch = next;
        return next.batchId;
    }

    mutable std::mutex m_mutex;
    std::deque<Command> m_pending;
    std::unordered_map<std::string, GTA_Unlock_Operation_Record> m_records;
    GTA_Unlock_Batch_Snapshot m_batch;
    std::uint64_t m_nextBatchId = 1;
};

}
