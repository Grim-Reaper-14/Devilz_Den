#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

#include <sol/sol.hpp>

namespace Devilz::Scripting::Lua
{
struct Lua_Command_Result
{
    bool succeeded{};
    std::string message;
};

class Lua_Commands final
{
public:
    bool Register(
        std::uint64_t ownerScriptId,
        std::string name,
        std::string description,
        sol::protected_function callback);

    [[nodiscard]] Lua_Command_Result Execute(std::string_view name);
    [[nodiscard]] bool Contains(std::string_view name) const;
    [[nodiscard]] std::size_t Count() const noexcept;

    std::size_t RemoveByOwner(std::uint64_t ownerScriptId);
    void Clear() noexcept;

private:
    struct Entry
    {
        std::uint64_t ownerScriptId{};
        std::string description;
        sol::protected_function callback;
    };

    std::unordered_map<std::string, Entry> m_commands;
};
}
