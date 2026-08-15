#include "Lua_Event_Manager.hpp"

#include <algorithm>
#include <utility>

namespace Devilz::Scripting::Lua
{
Lua_Event_Manager::Subscription_Id Lua_Event_Manager::Subscribe(
    std::uint64_t ownerScriptId,
    std::string name,
    sol::protected_function callback)
{
    if (name.empty() || !callback.valid())
        return 0;

    const auto id = m_nextId++;
    m_events[std::move(name)].push_back({id, ownerScriptId, std::move(callback)});
    return id;
}

bool Lua_Event_Manager::Unsubscribe(
    std::uint64_t ownerScriptId,
    Subscription_Id id) noexcept
{
    if (id == 0)
        return false;

    for (auto eventIt = m_events.begin(); eventIt != m_events.end(); ++eventIt) {
        auto& subscriptions = eventIt->second;
        const auto subscriptionIt = std::find_if(
            subscriptions.begin(),
            subscriptions.end(),
            [ownerScriptId, id](const Subscription& subscription) {
                return subscription.id == id && subscription.ownerScriptId == ownerScriptId;
            });

        if (subscriptionIt == subscriptions.end())
            continue;

        subscriptions.erase(subscriptionIt);
        if (subscriptions.empty())
            m_events.erase(eventIt);
        return true;
    }

    return false;
}

std::size_t Lua_Event_Manager::RemoveByOwner(std::uint64_t ownerScriptId) noexcept
{
    std::size_t removed = 0;

    for (auto eventIt = m_events.begin(); eventIt != m_events.end();) {
        auto& subscriptions = eventIt->second;
        const auto oldSize = subscriptions.size();

        subscriptions.erase(
            std::remove_if(
                subscriptions.begin(),
                subscriptions.end(),
                [ownerScriptId](const Subscription& subscription) {
                    return subscription.ownerScriptId == ownerScriptId;
                }),
            subscriptions.end());

        removed += oldSize - subscriptions.size();
        if (subscriptions.empty())
            eventIt = m_events.erase(eventIt);
        else
            ++eventIt;
    }

    return removed;
}

Lua_Event_Emit_Result Lua_Event_Manager::Emit(std::string_view name)
{
    Lua_Event_Emit_Result result;
    const auto eventIt = m_events.find(std::string{name});
    if (eventIt == m_events.end())
        return result;

    // Snapshot callbacks so handlers may subscribe/unsubscribe while an event is dispatching.
    auto subscriptions = eventIt->second;
    for (auto& subscription : subscriptions) {
        auto callbackResult = subscription.callback();
        ++result.dispatched;

        if (callbackResult.valid())
            continue;

        ++result.failed;
        if (result.message.empty()) {
            const sol::error error = callbackResult;
            result.message = error.what();
        }
    }

    result.succeeded = result.failed == 0;
    return result;
}

std::size_t Lua_Event_Manager::Count() const noexcept
{
    std::size_t count = 0;
    for (const auto& [name, subscriptions] : m_events) {
        (void)name;
        count += subscriptions.size();
    }
    return count;
}

std::size_t Lua_Event_Manager::CountByOwner(std::uint64_t ownerScriptId) const noexcept
{
    std::size_t count = 0;
    for (const auto& [name, subscriptions] : m_events) {
        (void)name;
        count += static_cast<std::size_t>(std::count_if(
            subscriptions.begin(),
            subscriptions.end(),
            [ownerScriptId](const Subscription& subscription) {
                return subscription.ownerScriptId == ownerScriptId;
            }));
    }
    return count;
}

void Lua_Event_Manager::Clear() noexcept
{
    m_events.clear();
}
}
