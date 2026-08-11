#pragma once

#include <atomic>
#include <cstdint>
#include <limits>
#include <optional>

namespace Devilz::Integrations::GTA5_Enhanced
{
enum class GTA_Network_Join_Type : int
{
    JoinPublic = 0,
    NewPublic = 1,
    ClosedCrew = 2,
    Crew = 3,
    ClosedFriends = 6,
    FindFriend = 9,
    Solo = 10,
    InviteOnly = 11,
    JoinCrew = 12,
    ScTv = 13,
    LeaveOnline = -1
};

enum class GTA_Network_Session_Status : std::uint8_t
{
    Unavailable,
    Ready,
    Queued,
    Switching,
    Success,
    Failed
};

class GTA_Network_Session_State final
{
public:
    static GTA_Network_Session_State& Instance() noexcept
    {
        static GTA_Network_Session_State state;
        return state;
    }

    void SetRuntimeReady(bool ready) noexcept
    {
        m_runtimeReady.store(ready);
        if (!ready) {
            m_pending.store(NoRequest);
            m_status.store(GTA_Network_Session_Status::Unavailable);
            return;
        }

        if (m_status.load() == GTA_Network_Session_Status::Unavailable)
            m_status.store(GTA_Network_Session_Status::Ready);
    }

    [[nodiscard]] bool RuntimeReady() const noexcept
    {
        return m_runtimeReady.load();
    }

    [[nodiscard]] GTA_Network_Session_Status Status() const noexcept
    {
        return m_status.load();
    }

    [[nodiscard]] bool Request(GTA_Network_Join_Type type) noexcept
    {
        if (!RuntimeReady()) {
            m_status.store(GTA_Network_Session_Status::Unavailable);
            return false;
        }

        int expected = NoRequest;
        if (!m_pending.compare_exchange_strong(expected, static_cast<int>(type)))
            return false;

        m_status.store(GTA_Network_Session_Status::Queued);
        return true;
    }

    [[nodiscard]] std::optional<GTA_Network_Join_Type> TakePending() noexcept
    {
        const int value = m_pending.exchange(NoRequest);
        if (value == NoRequest)
            return std::nullopt;

        m_status.store(GTA_Network_Session_Status::Switching);
        return static_cast<GTA_Network_Join_Type>(value);
    }

    void Complete(bool success) noexcept
    {
        m_status.store(success ? GTA_Network_Session_Status::Success
                               : GTA_Network_Session_Status::Failed);
    }

private:
    static constexpr int NoRequest = (std::numeric_limits<int>::min)();

    GTA_Network_Session_State() = default;

    std::atomic_bool m_runtimeReady{false};
    std::atomic_int m_pending{NoRequest};
    std::atomic<GTA_Network_Session_Status> m_status{GTA_Network_Session_Status::Unavailable};
};
}
