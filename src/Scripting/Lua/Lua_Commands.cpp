#include "Lua_Commands.hpp"

namespace Devilz::Scripting::Lua
{
bool Lua_Commands::Register(
    std::uint64_t ownerScriptId,
    std::string name,
    std::string description,
    sol::protected_function callback)
{
    if (name.empty() || !callback.valid())
        return false;

    Entry entry;
    entry.ownerScriptId = ownerScriptId;
    entry.description = std::move(description);
    entry.callback = std::move(callback);

    m_commands.insert_or_assign(std::move(name), std::move(entry));
    return true;
}

Lua_Command_Result Lua_Commands::Execute(std::string_view name)
{
    const auto it = m_commands.find(std::string{name});
    if (it == m_commands.end())
        return {false, "Lua command not found"};

    auto result = it->second.callback();
    if (!result.valid()) {
        const sol::error error = result;
        return {false, error.what()};
    }

    return {true, "Lua command executed"};
}

bool Lua_Commands::Contains(std::string_view name) const
{
    return m_commands.contains(std::string{name});
}

std::size_t Lua_Commands::Count() const noexcept
{
    return m_commands.size();
}

std::size_t Lua_Commands::RemoveByOwner(std::uint64_t ownerScriptId)
{
    std::size_t removed = 0;
    for (auto it = m_commands.begin(); it != m_commands.end();) {
        if (it->second.ownerScriptId == ownerScriptId) {
            it = m_commands.erase(it);
            ++removed;
        } else {
            ++it;
        }
    }
    return removed;
}

void Lua_Commands::Clear() noexcept
{
    m_commands.clear();
}
}
