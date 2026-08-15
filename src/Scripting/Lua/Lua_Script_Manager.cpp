#include "Lua_Script_Manager.hpp"

#include "Events/Lua_Event_Manager.hpp"
#include "Features/Lua_Feature_Manager.hpp"
#include "Fingerprint/Lua_Fingerprint.hpp"
#include "Lua_Binding_Library_Manager.hpp"
#include "Lua_Commands.hpp"
#include "Lua_Engine_Manager.hpp"
#include "Settings/Lua_Setting_Manager.hpp"

#include <algorithm>
#include <utility>

namespace Devilz::Scripting::Lua
{
void Lua_Script_Manager::Configure(
    Lua_Engine_Manager* engines,
    Lua_Binding_Library_Manager* libraries,
    Lua_Binding_Context context) noexcept
{
    m_engines = engines;
    m_libraries = libraries;
    m_bindingContext = std::move(context);
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
    if (!ReadyToLoad())
        return nullptr;

    const auto id = m_nextId++;
    auto script = std::make_unique<Lua_Script>(id, path);
    BuildScriptEngine(*script);

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
    CleanupOwnerResources(id);
    (*it)->Unload();

    if (m_engines && engineId != 0)
        m_engines->DestroyEngine(engineId);

    m_scripts.erase(it);
    return true;
}

bool Lua_Script_Manager::ReloadScript(Lua_Script::Id id)
{
    auto* script = FindScript(id);
    if (!script || !ReadyToLoad())
        return false;

    const auto oldEngineId = script->EngineId();
    CleanupOwnerResources(id);
    script->Unload();

    if (oldEngineId != 0)
        m_engines->DestroyEngine(oldEngineId);

    return BuildScriptEngine(*script);
}

void Lua_Script_Manager::Tick()
{
    if (!m_engines)
        return;

    for (auto& script : m_scripts) {
        if (script->State() != Lua_Script_State::Running)
            continue;

        auto* engine = m_engines->FindEngine(script->EngineId());
        if (!engine) {
            script->MarkError("Lua script engine was destroyed");
            continue;
        }

        script->Tick(*engine);
    }
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

bool Lua_Script_Manager::ReadyToLoad() const noexcept
{
    return m_engines && m_libraries && m_bindingContext.commands &&
        m_bindingContext.events && m_bindingContext.fingerprints &&
        m_bindingContext.settings && m_bindingContext.features;
}

bool Lua_Script_Manager::BuildScriptEngine(Lua_Script& script)
{
    if (!ReadyToLoad()) {
        script.MarkError("Lua script manager is not configured");
        return false;
    }

    auto& engine = m_engines->CreateEngine(script.GetId());
    if (!engine.Ready()) {
        script.MarkError(std::string{engine.Status()});
        m_engines->DestroyEngine(engine.GetId());
        script.DetachEngine();
        return false;
    }

    if (!m_libraries->BindAll(engine, m_bindingContext)) {
        script.MarkError("Failed to bind Lua libraries");
        m_engines->DestroyEngine(engine.GetId());
        script.DetachEngine();
        return false;
    }

    if (!script.Load(engine, *m_bindingContext.fingerprints)) {
        CleanupOwnerResources(script.GetId());
        m_engines->DestroyEngine(engine.GetId());
        script.DetachEngine();
        return false;
    }

    return true;
}

void Lua_Script_Manager::CleanupOwnerResources(Lua_Script::Id id) noexcept
{
    if (m_bindingContext.features)
        m_bindingContext.features->RemoveByOwner(id);
    if (m_bindingContext.settings)
        m_bindingContext.settings->RemoveByOwner(id);
    if (m_bindingContext.events)
        m_bindingContext.events->RemoveByOwner(id);
    if (m_bindingContext.commands)
        m_bindingContext.commands->RemoveByOwner(id);
}
}
