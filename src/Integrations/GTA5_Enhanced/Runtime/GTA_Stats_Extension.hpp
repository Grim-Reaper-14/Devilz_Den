#pragma once

#include "Integrations/GTA5_Enhanced/Types/Stats/GTA_Stat_Data.hpp"

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>

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

class GTA_Stats_State final
{
public:
    static GTA_Stats_State& Instance() noexcept;

    GTA_Stats_State(const GTA_Stats_State&) = delete;
    GTA_Stats_State& operator=(const GTA_Stats_State&) = delete;

    void RequestRead(std::string statName);
    void RequestWrite(std::string statName, std::string value);
    [[nodiscard]] GTA_Stat_Snapshot Snapshot() const;
    void Reset();

    // Internal command transport consumed only by the game-thread extension.
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

    [[nodiscard]] bool Consume(Command& command);
    void Publish(GTA_Stat_Snapshot snapshot);

    friend void TickStatsExtension(GTA_Native_Manager& natives) noexcept;
    friend void ResetStatsExtension() noexcept;

    mutable std::mutex m_mutex;
    std::optional<Command> m_pending;
    GTA_Stat_Snapshot m_snapshot;
    std::uint64_t m_nextRequestId = 1;
};

[[nodiscard]] const char* GTAStatTypeName(GTA_Stat_Data::Type type) noexcept;
[[nodiscard]] const char* GTAStatRequestStatusName(GTA_Stat_Request_Status status) noexcept;

void TickStatsExtension(GTA_Native_Manager& natives) noexcept;
void ResetStatsExtension() noexcept;
}
