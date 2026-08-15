#include "Lua_Script_Manager.hpp"

#include "Lua_Binding_Library_Manager.hpp"
#include "Lua_Commands.hpp"
#include "Lua_Engine_Manager.hpp"

#include <algorithm>

namespace Devilz::Scripting::Lua
{
void Lua_Script_Manager::Configure(
    Lua_Engine_Manager* engines,
    Lua_Binding_Library_Manager* libraries,
    Lua_Commands* commands) noexcept
{
    m_engines = engines;
    m_libraries = libraries;
    m_commands = commands;
}

std::size_t Lua_Script_Manager::DiscoverScripts(const std::filesystem::path& directory)
{
    m_discoveredScripts.clear();
    if (!std::filesystem::exists(directory))
        return 0;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".lua")
            continue;
        m_discoveredScripts.push_back(entry.path());
    }

    std::sort(m_discoveredScripts.begin(), m_discoveredScripts.end());
    return m_discoveredScripts.size();
}

Lua_Script* Lua_Script_Manager::LoadScript(const std::filesystem::path& path)
{
    if (!m_engines || !m_libraries || !m_commands)
        return nullptr;

    const auto id = m_nextId++;
    auto script = std::make_unique<Lua_Script>(id, path);
    auto& engine = m_engines->CreateEngine(id);

    if (!engine.Ready()) {
        script->MarkError(std::string{engine.Status()});
        m_engines->DestroyEngine(engine.GetId());
    } else if (!m_libraries->BindAll(engine, *m_commands)) {
        script->MarkError("Failed to bind Lua libraries");
        m_engines->DestroyEngine(engine.GetId());
    } else {
        script->Load(engine);
    }

    auto* result = script.get();
    m_scripts.push_back(std::move(script));
    return result;
}

bool Lua_Script_Manager::UnloadScript(Lua_Script::Id id) noexcept
{
    const auto it = std::find_if(
        m_scripts.begin(),
        m_scripts.end(),
        [id](const auto& script) { return script->GetId() == id; });
    if (it == m_scripts.end())
        return false;

    const auto engineId = (*it)->EngineId();
    if (m_commands)
        m_commands->RemoveByOwner(id);
    (*it)->Unload();
    if (m_engines && engineId != 0)
        m_engines->DestroyEngine(engineId);

    m_scripts.erase(it);
    return true;
}

bool Lua_Script_Manager::ReloadScript(Lua_Script::Id id)
{
    auto* script = FindScript(id);
    if (!script)
        return false;

    const auto path = script->Path();
    if (!UnloadScript(id))
        return false;

    auto* reloaded = LoadScript(path);
    return reloaded && reloaded->State() == Lua_Script_State::Running;
}

void Lua_Script_Manager::Tick()
{
    for (auto& script : m_scripts)
        script->Tick();
}

void Lua_Script_Manager::UnloadAll() noexcept
{
    while (!m_scripts.empty())
        UnloadScript(m_scripts.back()->GetId());
    m_discoveredScripts.clear();
}

Lua_Script* Lua_Script_Manager::FindScript(Lua_Script::Id id) noexcept
{
    const auto it = std::find_if(
        m_scripts.begin(),
        m_scripts.end(),
        [id](const auto& script) { return script->GetId() == id; });
    return it == m_scripts.end() ? nullptr : it->get();
}

const std::vector<std::unique_ptr<Lua_Script>>& Lua_Script_Manager::Scripts() const noexcept
{
    return m_scripts;
}

const std::vector<std::filesystem::path>& Lua_Script_Manager::DiscoveredScripts() const noexcept
{
    return m_discoveredScripts;
}
}
