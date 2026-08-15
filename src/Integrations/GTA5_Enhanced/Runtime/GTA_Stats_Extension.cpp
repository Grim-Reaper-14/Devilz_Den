#include "GTA_Stats_Extension.hpp"

#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Manager.hpp"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <string>
#include <string_view>
#include <utility>

namespace Devilz::Integrations::GTA5_Enhanced
{
namespace
{
// Enhanced 1.73 native hashes, cross-checked against the current YimMenuV2
// generated crossmap. Keep these in sync with GTA_Native_Manager's probes.
constexpr GTA_Native_Hash StatGetInt = 0xDF7F16323520B858ULL;
constexpr GTA_Native_Hash StatGetFloat = 0x2F0966A034F5ADC6ULL;
constexpr GTA_Native_Hash StatGetBool = 0xF249567F2E83E093ULL;
constexpr GTA_Native_Hash StatGetString = 0xCEA81DACD6DA3ADBULL;
constexpr GTA_Native_Hash StatSetInt = 0x1164A75E490C27B6ULL;
constexpr GTA_Native_Hash StatSetFloat = 0x4F8678C02360C3D2ULL;
constexpr GTA_Native_Hash StatSetBool = 0xF1D0B0CE940F620DULL;
constexpr GTA_Native_Hash StatSetString = 0x1A43F9BE4B6AAB67ULL;
constexpr std::size_t MaxStatStringLength = 255;

[[nodiscard]] char Lower(char value) noexcept
{
    return value >= 'A' && value <= 'Z' ? static_cast<char>(value + ('a' - 'A')) : value;
}

[[nodiscard]] std::string_view Trim(std::string_view value) noexcept
{
    auto space = [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
    while (!value.empty() && space(value.front())) value.remove_prefix(1);
    while (!value.empty() && space(value.back())) value.remove_suffix(1);
    return value;
}

[[nodiscard]] bool StartsI(std::string_view value, std::string_view prefix) noexcept
{
    if (value.size() < prefix.size()) return false;
    for (std::size_t i = 0; i < prefix.size(); ++i)
        if (Lower(value[i]) != Lower(prefix[i])) return false;
    return true;
}

[[nodiscard]] std::uint32_t Joaat(std::string_view text) noexcept
{
    std::uint32_t hash = 0;
    for (char c : text) {
        hash += static_cast<std::uint8_t>(Lower(c));
        hash += hash << 10U;
        hash ^= hash >> 6U;
    }
    hash += hash << 3U;
    hash ^= hash >> 11U;
    return hash + (hash << 15U);
}

[[nodiscard]] bool ResolveCharacterIndex(GTA_Native_Manager& natives, int& character) noexcept
{
    std::int32_t value = 0;
    const auto result = natives.InvokeHash<bool>(StatGetInt, Joaat("MPPLY_LAST_MP_CHAR"), &value, -1);
    if (!result.has_value() || !result.value()) return false;
    character = value == 1 ? 1 : 0;
    return true;
}

struct NativeStatRead
{
    std::string normalizedName;
    std::uint32_t hash = 0;
    GTA_Stat_Data::Type type = GTA_Stat_Data::Type::Int;
    std::int32_t intValue = 0;
    float floatValue = 0.0F;
    bool boolValue = false;
    std::string stringValue;
    std::string value;
};

[[nodiscard]] bool ReadExactType(
    GTA_Native_Manager& natives,
    std::uint32_t hash,
    GTA_Stat_Data::Type type,
    NativeStatRead& result)
{
    result.hash = hash;
    result.type = type;
    if (type == GTA_Stat_Data::Type::Int) {
        std::int32_t value = 0;
        const auto read = natives.InvokeHash<bool>(StatGetInt, hash, &value, -1);
        if (!read.has_value() || !read.value()) return false;
        result.intValue = value;
        result.value = std::to_string(value);
        return true;
    }
    if (type == GTA_Stat_Data::Type::Bool) {
        std::int32_t value = 0;
        const auto read = natives.InvokeHash<bool>(StatGetBool, hash, &value, -1);
        if (!read.has_value() || !read.value()) return false;
        result.boolValue = value != 0;
        result.value = result.boolValue ? "true" : "false";
        return true;
    }
    if (type == GTA_Stat_Data::Type::Float) {
        float value = 0.0F;
        const auto read = natives.InvokeHash<bool>(StatGetFloat, hash, &value, -1);
        if (!read.has_value() || !read.value() || !std::isfinite(value)) return false;
        result.floatValue = value;
        result.value = std::to_string(value);
        return true;
    }
    if (type == GTA_Stat_Data::Type::String) {
        const auto read = natives.InvokeHash<const char*>(StatGetString, hash, -1);
        if (!read.has_value() || !read.value()) return false;

        std::size_t length = 0;
        while (length < MaxStatStringLength && read.value()[length] != '\0')
            ++length;
        if (length == MaxStatStringLength && read.value()[length] != '\0')
            return false;

        result.stringValue.assign(read.value(), length);
        result.value = result.stringValue;
        return true;
    }
    return false;
}

[[nodiscard]] bool TryReadHash(GTA_Native_Manager& natives, std::string name, NativeStatRead& result)
{
    const auto hash = Joaat(name);
    if (!hash) return false;

    NativeStatRead candidate{};
    candidate.normalizedName = name;
    if (ReadExactType(natives, hash, GTA_Stat_Data::Type::Int, candidate)) {
        result = std::move(candidate);
        return true;
    }

    candidate = {};
    candidate.normalizedName = name;
    if (ReadExactType(natives, hash, GTA_Stat_Data::Type::Float, candidate)) {
        result = std::move(candidate);
        return true;
    }

    candidate = {};
    candidate.normalizedName = name;
    if (ReadExactType(natives, hash, GTA_Stat_Data::Type::Bool, candidate)) {
        result = std::move(candidate);
        return true;
    }

    candidate = {};
    candidate.normalizedName = name;
    if (ReadExactType(natives, hash, GTA_Stat_Data::Type::String, candidate)) {
        result = std::move(candidate);
        return true;
    }
    return false;
}

[[nodiscard]] bool ResolveAndRead(
    GTA_Native_Manager& natives,
    std::string_view input,
    NativeStatRead& result,
    std::string& error)
{
    input = Trim(input);
    if (!input.empty() && input.front() == '$') input = Trim(input.substr(1));
    if (input.empty() || input.size() > 127) {
        error = "Enter a valid stat name";
        return false;
    }

    std::string name(input);
    int character = 0;
    bool characterKnown = false;
    if (name.size() > 3 && StartsI(name, "MPX_")) {
        characterKnown = ResolveCharacterIndex(natives, character);
        if (!characterKnown) {
            error = "Active MP character could not be resolved through STAT_GET_INT";
            return false;
        }
        name[2] = static_cast<char>('0' + character);
    }

    if (TryReadHash(natives, name, result)) return true;

    const bool explicitMp = name.size() > 3 && Lower(name[0]) == 'm' && Lower(name[1]) == 'p' &&
        (name[2] == '0' || name[2] == '1' || StartsI(name, "MPPLY_"));
    if (explicitMp) {
        error = "Native stat read failed or this stat type is not supported by the editor";
        return false;
    }

    if (!characterKnown) characterKnown = ResolveCharacterIndex(natives, character);
    if (!characterKnown) {
        error = "Native stat read failed and the active MP character could not be resolved";
        return false;
    }

    const std::string prefixed = (character == 0 ? "MP0_" : "MP1_") + name;
    if (TryReadHash(natives, prefixed, result)) return true;

    error = "Native stat read failed or this stat type is not supported by the editor";
    return false;
}

[[nodiscard]] bool ParseInt(std::string_view text, std::int32_t& value) noexcept
{
    std::string copy(Trim(text));
    if (copy.empty()) return false;
    char* end = nullptr;
    errno = 0;
    const auto parsed = std::strtoll(copy.c_str(), &end, 10);
    if (errno == ERANGE || end != copy.c_str() + copy.size() ||
        parsed < (std::numeric_limits<std::int32_t>::min)() ||
        parsed > (std::numeric_limits<std::int32_t>::max)()) return false;
    value = static_cast<std::int32_t>(parsed);
    return true;
}

[[nodiscard]] bool ParseBool(std::string_view text, bool& value) noexcept
{
    const auto t = Trim(text);
    if (t == "1" || (t.size() == 4 && StartsI(t, "true"))) { value = true; return true; }
    if (t == "0" || (t.size() == 5 && StartsI(t, "false"))) { value = false; return true; }
    return false;
}

[[nodiscard]] bool ParseFloat(std::string_view text, float& value) noexcept
{
    std::string copy(Trim(text));
    if (copy.empty()) return false;
    char* end = nullptr;
    errno = 0;
    const auto parsed = std::strtof(copy.c_str(), &end);
    if (errno == ERANGE || end != copy.c_str() + copy.size() || !std::isfinite(parsed))
        return false;
    value = parsed;
    return true;
}

[[nodiscard]] bool FloatsMatch(float left, float right) noexcept
{
    const auto scale = (std::max)(1.0F, (std::max)(std::fabs(left), std::fabs(right)));
    return std::fabs(left - right) <= std::numeric_limits<float>::epsilon() * 4.0F * scale;
}

[[nodiscard]] GTA_Stat_Snapshot Execute(GTA_Native_Manager& natives, const GTA_Stats_State::Command& command)
{
    GTA_Stat_Snapshot snapshot{};
    snapshot.requestId = command.id;
    snapshot.requestedName = command.statName;
    if (!natives.Ready()) {
        snapshot.status = GTA_Stat_Request_Status::RuntimeUnavailable;
        snapshot.detail = "GTA native manager is unavailable";
        return snapshot;
    }

    NativeStatRead current{};
    std::string error;
    if (!ResolveAndRead(natives, command.statName, current, error)) {
        snapshot.status = GTA_Stat_Request_Status::NotFound;
        snapshot.detail = std::move(error);
        return snapshot;
    }

    snapshot.normalizedName = current.normalizedName;
    snapshot.hash = current.hash;
    snapshot.type = current.type;
    snapshot.typeKnown = true;
    snapshot.writeSupported = current.type == GTA_Stat_Data::Type::Int ||
        current.type == GTA_Stat_Data::Type::Float ||
        current.type == GTA_Stat_Data::Type::Bool ||
        current.type == GTA_Stat_Data::Type::String;
    snapshot.value = current.value;

    if (command.kind == GTA_Stats_State::Command_Kind::Read) {
        snapshot.status = GTA_Stat_Request_Status::Succeeded;
        snapshot.detail = "Stat read through validated GTA native handlers; CStatsMgr is not required";
        return snapshot;
    }

    bool dispatched = false;
    std::int32_t requestedInt = 0;
    float requestedFloat = 0.0F;
    bool requestedBool = false;
    std::string requestedString;
    if (current.type == GTA_Stat_Data::Type::Int) {
        if (!ParseInt(command.value, requestedInt)) {
            snapshot.status = GTA_Stat_Request_Status::InvalidValue;
            snapshot.detail = "Expected a signed 32-bit integer";
            return snapshot;
        }
        dispatched = natives.InvokeHash<void>(StatSetInt, current.hash, requestedInt, true);
    } else if (current.type == GTA_Stat_Data::Type::Float) {
        if (!ParseFloat(command.value, requestedFloat)) {
            snapshot.status = GTA_Stat_Request_Status::InvalidValue;
            snapshot.detail = "Expected a finite floating-point value";
            return snapshot;
        }
        dispatched = natives.InvokeHash<void>(StatSetFloat, current.hash, requestedFloat, true);
    } else if (current.type == GTA_Stat_Data::Type::Bool) {
        if (!ParseBool(command.value, requestedBool)) {
            snapshot.status = GTA_Stat_Request_Status::InvalidValue;
            snapshot.detail = "Expected true, false, 1, or 0";
            return snapshot;
        }
        dispatched = natives.InvokeHash<void>(StatSetBool, current.hash, requestedBool, true);
    } else if (current.type == GTA_Stat_Data::Type::String) {
        if (command.value.size() > MaxStatStringLength) {
            snapshot.status = GTA_Stat_Request_Status::InvalidValue;
            snapshot.detail = "String values are limited to 255 characters";
            return snapshot;
        }
        requestedString = command.value;
        dispatched = natives.InvokeHash<void>(
            StatSetString,
            current.hash,
            requestedString.c_str(),
            true);
    } else {
        snapshot.status = GTA_Stat_Request_Status::UnsupportedType;
        snapshot.detail = "Native stat editor currently supports INT, FLOAT, BOOL, and STRING writes";
        return snapshot;
    }

    if (!dispatched) {
        snapshot.status = GTA_Stat_Request_Status::Failed;
        snapshot.detail = "Stat write handler is unavailable";
        return snapshot;
    }

    NativeStatRead verified{};
    verified.normalizedName = current.normalizedName;
    if (!ReadExactType(natives, current.hash, current.type, verified)) {
        snapshot.status = GTA_Stat_Request_Status::Failed;
        snapshot.detail = "Stat write was dispatched but readback failed";
        return snapshot;
    }

    bool matched = false;
    switch (current.type) {
    case GTA_Stat_Data::Type::Int:
        matched = verified.intValue == requestedInt;
        break;
    case GTA_Stat_Data::Type::Float:
        matched = FloatsMatch(verified.floatValue, requestedFloat);
        break;
    case GTA_Stat_Data::Type::Bool:
        matched = verified.boolValue == requestedBool;
        break;
    case GTA_Stat_Data::Type::String:
        matched = verified.stringValue == requestedString;
        break;
    default:
        break;
    }
    snapshot.value = verified.value;
    if (!matched) {
        snapshot.status = GTA_Stat_Request_Status::Failed;
        snapshot.detail = "Stat write was dispatched but GTA readback did not match the requested value";
        return snapshot;
    }

    snapshot.status = GTA_Stat_Request_Status::Succeeded;
    snapshot.detail = "Stat write verified by native readback; server-authoritative stats may still be replaced later";
    return snapshot;
}
}

GTA_Stats_State& GTA_Stats_State::Instance() noexcept
{
    static GTA_Stats_State state;
    return state;
}

std::uint64_t GTA_Stats_State::RequestRead(std::string statName)
{
    return Queue(Command_Kind::Read, std::move(statName), {});
}

std::uint64_t GTA_Stats_State::RequestWrite(std::string statName, std::string value)
{
    return Queue(Command_Kind::Write, std::move(statName), std::move(value));
}

std::uint64_t GTA_Stats_State::Queue(
    Command_Kind kind,
    std::string statName,
    std::string value)
{
    std::scoped_lock lock(m_mutex);
    const auto id = m_nextRequestId++;

    if (m_pending.size() + (m_inFlight.has_value() ? 1U : 0U) >= MaxQueuedCommands) {
        GTA_Stat_Snapshot rejected{};
        rejected.revision = ++m_revision;
        rejected.requestId = id;
        rejected.status = GTA_Stat_Request_Status::QueueFull;
        rejected.requestedName = std::move(statName);
        rejected.detail = "Regular stat queue is full";
        ++m_completed;
        ++m_failed;
        ++m_rejected;
        m_snapshot = rejected;
        PushHistoryLocked(std::move(rejected));
        return id;
    }

    Command command{id, kind, std::move(statName), std::move(value)};
    m_pending.push_back(command);
    m_snapshot = QueuedSnapshotLocked(command);
    m_snapshot.revision = ++m_revision;
    return id;
}

GTA_Stat_Snapshot GTA_Stats_State::QueuedSnapshotLocked(const Command& command) const
{
    GTA_Stat_Snapshot snapshot{};
    snapshot.requestId = command.id;
    snapshot.status = GTA_Stat_Request_Status::Queued;
    snapshot.requestedName = command.statName;
    snapshot.detail = command.kind == Command_Kind::Read
        ? "Read queued for the GTA game thread"
        : "Write queued for the GTA game thread";
    return snapshot;
}

GTA_Stat_Snapshot GTA_Stats_State::Snapshot() const
{
    std::scoped_lock lock(m_mutex);
    return m_snapshot;
}

std::optional<GTA_Stat_Snapshot> GTA_Stats_State::Snapshot(std::uint64_t requestId) const
{
    if (requestId == 0)
        return std::nullopt;

    std::scoped_lock lock(m_mutex);
    if (m_inFlight && m_inFlight->id == requestId) {
        auto snapshot = QueuedSnapshotLocked(*m_inFlight);
        snapshot.revision = m_revision;
        snapshot.detail = "Stat request is executing on the GTA game thread";
        return snapshot;
    }

    for (const auto& command : m_pending) {
        if (command.id == requestId) {
            auto snapshot = QueuedSnapshotLocked(command);
            snapshot.revision = m_revision;
            return snapshot;
        }
    }

    for (auto it = m_history.rbegin(); it != m_history.rend(); ++it) {
        if (it->requestId == requestId)
            return *it;
    }

    if (m_snapshot.requestId == requestId)
        return m_snapshot;
    return std::nullopt;
}

GTA_Stats_Queue_Snapshot GTA_Stats_State::QueueSnapshot() const
{
    std::scoped_lock lock(m_mutex);
    GTA_Stats_Queue_Snapshot snapshot{};
    snapshot.revision = m_revision;
    snapshot.pending = m_pending.size();
    snapshot.inFlight = m_inFlight.has_value() ? 1U : 0U;
    snapshot.capacity = MaxQueuedCommands;
    snapshot.completed = m_completed;
    snapshot.succeeded = m_succeeded;
    snapshot.failed = m_failed;
    snapshot.cancelled = m_cancelled;
    snapshot.rejected = m_rejected;
    return snapshot;
}

std::vector<GTA_Stat_Snapshot> GTA_Stats_State::RecentHistory(std::size_t maxCount) const
{
    std::scoped_lock lock(m_mutex);
    maxCount = (std::min)(maxCount, m_history.size());
    std::vector<GTA_Stat_Snapshot> history;
    history.reserve(maxCount);
    for (auto it = m_history.rbegin(); it != m_history.rend() && history.size() < maxCount; ++it)
        history.push_back(*it);
    return history;
}

bool GTA_Stats_State::CancelPending(std::uint64_t requestId)
{
    std::scoped_lock lock(m_mutex);
    const auto it = std::find_if(
        m_pending.begin(),
        m_pending.end(),
        [requestId](const Command& command) { return command.id == requestId; });
    if (it == m_pending.end())
        return false;

    GTA_Stat_Snapshot cancelled{};
    cancelled.revision = ++m_revision;
    cancelled.requestId = it->id;
    cancelled.status = GTA_Stat_Request_Status::Cancelled;
    cancelled.requestedName = it->statName;
    cancelled.detail = "Stat request cancelled before game-thread execution";
    m_pending.erase(it);
    ++m_completed;
    ++m_cancelled;
    if (m_snapshot.requestId == requestId)
        m_snapshot = cancelled;
    PushHistoryLocked(std::move(cancelled));
    return true;
}

std::size_t GTA_Stats_State::ClearPending()
{
    std::scoped_lock lock(m_mutex);
    const std::size_t count = m_pending.size();
    for (const auto& command : m_pending) {
        GTA_Stat_Snapshot cancelled{};
        cancelled.revision = ++m_revision;
        cancelled.requestId = command.id;
        cancelled.status = GTA_Stat_Request_Status::Cancelled;
        cancelled.requestedName = command.statName;
        cancelled.detail = "Stat request cleared before game-thread execution";
        ++m_completed;
        ++m_cancelled;
        if (m_snapshot.requestId == command.id)
            m_snapshot = cancelled;
        PushHistoryLocked(std::move(cancelled));
    }
    m_pending.clear();
    return count;
}

void GTA_Stats_State::Reset()
{
    std::scoped_lock lock(m_mutex);
    m_pending.clear();
    m_inFlight.reset();
    m_history.clear();
    m_snapshot = {};
    m_nextRequestId = 1;
    m_revision = 0;
    m_completed = 0;
    m_succeeded = 0;
    m_failed = 0;
    m_cancelled = 0;
    m_rejected = 0;
}

bool GTA_Stats_State::Consume(Command& command)
{
    std::scoped_lock lock(m_mutex);
    if (m_inFlight || m_pending.empty())
        return false;
    m_inFlight = std::move(m_pending.front());
    m_pending.pop_front();
    command = *m_inFlight;
    ++m_revision;
    return true;
}

void GTA_Stats_State::PushHistoryLocked(GTA_Stat_Snapshot snapshot)
{
    m_history.push_back(std::move(snapshot));
    while (m_history.size() > MaxHistoryEntries)
        m_history.pop_front();
}

void GTA_Stats_State::Publish(GTA_Stat_Snapshot snapshot)
{
    std::scoped_lock lock(m_mutex);
    if (m_inFlight && m_inFlight->id == snapshot.requestId)
        m_inFlight.reset();

    snapshot.revision = ++m_revision;
    ++m_completed;
    if (snapshot.status == GTA_Stat_Request_Status::Succeeded)
        ++m_succeeded;
    else if (snapshot.status == GTA_Stat_Request_Status::Cancelled)
        ++m_cancelled;
    else
        ++m_failed;

    if (snapshot.requestId >= m_snapshot.requestId)
        m_snapshot = snapshot;
    PushHistoryLocked(std::move(snapshot));
}

const char* GTAStatTypeName(GTA_Stat_Data::Type type) noexcept
{
    switch (type) {
    case GTA_Stat_Data::Type::Int: return "INT";
    case GTA_Stat_Data::Type::Float: return "FLOAT";
    case GTA_Stat_Data::Type::String: return "STRING";
    case GTA_Stat_Data::Type::Bool: return "BOOL";
    case GTA_Stat_Data::Type::UInt8: return "UINT8";
    case GTA_Stat_Data::Type::UInt16: return "UINT16";
    case GTA_Stat_Data::Type::UInt32: return "UINT32";
    case GTA_Stat_Data::Type::UInt64: return "UINT64";
    case GTA_Stat_Data::Type::Date: return "DATE";
    case GTA_Stat_Data::Type::Pos: return "POS";
    case GTA_Stat_Data::Type::Int64: return "INT64";
    default: return "UNKNOWN";
    }
}

const char* GTAStatRequestStatusName(GTA_Stat_Request_Status status) noexcept
{
    switch (status) {
    case GTA_Stat_Request_Status::Idle: return "Idle";
    case GTA_Stat_Request_Status::Queued: return "Queued";
    case GTA_Stat_Request_Status::Succeeded: return "Succeeded";
    case GTA_Stat_Request_Status::RuntimeUnavailable: return "Runtime unavailable";
    case GTA_Stat_Request_Status::NotFound: return "Not found";
    case GTA_Stat_Request_Status::UnsupportedType: return "Unsupported type";
    case GTA_Stat_Request_Status::ControlledByNetShop: return "Netshop controlled";
    case GTA_Stat_Request_Status::InvalidValue: return "Invalid value";
    case GTA_Stat_Request_Status::QueueFull: return "Queue full";
    case GTA_Stat_Request_Status::Cancelled: return "Cancelled";
    case GTA_Stat_Request_Status::Failed: return "Failed";
    default: return "Unknown";
    }
}

std::size_t TickStatsExtension(GTA_Native_Manager& natives, std::size_t budget) noexcept
{
    if (budget == 0)
        return 0;

    auto& state = GTA_Stats_State::Instance();
    std::size_t processed = 0;
    while (processed < budget) {
        GTA_Stats_State::Command command{};
        if (!state.Consume(command))
            break;

        try {
            state.Publish(Execute(natives, command));
        } catch (...) {
            try {
                GTA_Stat_Snapshot result{};
                result.requestId = command.id;
                result.requestedName = command.statName;
                result.status = GTA_Stat_Request_Status::Failed;
                result.detail = "Stat operation failed on the GTA game thread";
                state.Publish(std::move(result));
            } catch (...) {
            }
        }
        ++processed;
    }
    return processed;
}

void ResetStatsExtension() noexcept
{
    try {
        GTA_Stats_State::Instance().Reset();
    } catch (...) {
    }
}
}
