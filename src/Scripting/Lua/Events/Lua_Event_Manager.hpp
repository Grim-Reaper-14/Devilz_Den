#pragma once

#include <sol/sol.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Devilz::Scripting::Lua
{
struct Lua_Event_Emit_Result
{
    bool succeeded{true};
    std::size_t dispatched{};
    std::size_t failed{};
    std::string message;
};

class Lua_Event_Manager final
{
public:
    using Subscription_Id = std::uint64_t;

    Subscription_Id Subscribe(
        std::uint64_t ownerScriptId,
        std::string name,
        sol::protected_function callback);

    bool Unsubscribe(std::uint64_t ownerScriptId, Subscription_Id id) noexcept;
    std::size_t RemoveByOwner(std::uint64_t ownerScriptId) noexcept;

    [[nodiscard]] Lua_Event_Emit_Result Emit(std::string_view name);
    [[nodiscard]] std::size_t Count() const noexcept;
    [[nodiscard]] std::size_t CountByOwner(std::uint64_t ownerScriptId) const noexcept;

    void Clear() noexcept;

private:
    struct Subscription
    {
        Subscription_Id id{};
        std::uint64_t ownerScriptId{};
        sol::protected_function callback;
    };

    Subscription_Id m_nextId{1};
    std::unordered_map<std::string, std::vector<Subscription>> m_events;
};
}
