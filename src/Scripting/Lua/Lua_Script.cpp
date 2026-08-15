#include "Lua_Script.hpp"

#include "Lua_Engine.hpp"

#include <sol/sol.hpp>

#include <utility>

namespace Devilz::Scripting::Lua
{
Lua_Script::Lua_Script(Id id, std::filesystem::path path)
    : m_id(id), m_path(std::move(path))
{
}

bool Lua_Script::Load(Lua_Engine& engine)
{
    m_state = Lua_Script_State::Loading;
    m_lastError.clear();
    m_engineId = engine.GetId();

    if (!engine.Ready()) {
        MarkError(std::string{engine.Status()});
        return false;
    }

    auto& state = engine.State();
    const sol::object devilzObject = state["devilz"];
    sol::table devilz = devilzObject.is<sol::table>()
        ? devilzObject.as<sol::table>()
        : state.create_named_table("devilz");

    auto script = state.create_table();
    script["id"] = m_id;
    script["path"] = m_path.string();
    devilz["script"] = script;

    auto result = state.safe_script_file(m_path.string(), sol::script_pass_on_error);
    if (!result.valid()) {
        const sol::error error = result;
        MarkError(error.what());
        return false;
    }

    m_state = Lua_Script_State::Running;
    return true;
}

void Lua_Script::Unload() noexcept
{
    m_state = Lua_Script_State::Unloaded;
    m_engineId = 0;
}

void Lua_Script::Tick()
{
    if (m_state != Lua_Script_State::Running)
        return;
}

void Lua_Script::MarkError(std::string message)
{
    m_lastError = std::move(message);
    m_state = Lua_Script_State::Error;
}

Lua_Script::Id Lua_Script::GetId() const noexcept
{
    return m_id;
}

const std::filesystem::path& Lua_Script::Path() const noexcept
{
    return m_path;
}

Lua_Script_State Lua_Script::State() const noexcept
{
    return m_state;
}

const std::string& Lua_Script::LastError() const noexcept
{
    return m_lastError;
}

std::uint64_t Lua_Script::EngineId() const noexcept
{
    return m_engineId;
}
}
