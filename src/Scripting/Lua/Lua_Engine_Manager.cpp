#include "Lua_Engine_Manager.hpp"

namespace Devilz::Scripting::Lua
{
Lua_Engine& Lua_Engine_Manager::CreateEngine(std::uint64_t ownerScriptId)
{
    const auto id = m_nextId++;
    auto engine = std::make_unique<Lua_Engine>(id, ownerScriptId);
    engine->Initialize();

    auto* result = engine.get();
    m_engines.emplace(id, std::move(engine));
    return *result;
}

bool Lua_Engine_Manager::DestroyEngine(Lua_Engine::Id id) noexcept
{
    const auto it = m_engines.find(id);
    if (it == m_engines.end())
        return false;

    it->second->Shutdown();
    m_engines.erase(it);
    return true;
}

Lua_Engine* Lua_Engine_Manager::FindEngine(Lua_Engine::Id id) noexcept
{
    const auto it = m_engines.find(id);
    return it == m_engines.end() ? nullptr : it->second.get();
}

const Lua_Engine* Lua_Engine_Manager::FindEngine(Lua_Engine::Id id) const noexcept
{
    const auto it = m_engines.find(id);
    return it == m_engines.end() ? nullptr : it->second.get();
}

std::size_t Lua_Engine_Manager::Count() const noexcept
{
    return m_engines.size();
}

void Lua_Engine_Manager::Clear() noexcept
{
    for (auto& [id, engine] : m_engines) {
        (void)id;
        engine->Shutdown();
    }
    m_engines.clear();
}
}
