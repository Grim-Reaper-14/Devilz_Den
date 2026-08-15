#include "Lua_Binding_Library_Manager.hpp"

#include "Lua_Commands.hpp"
#include "Lua_Engine.hpp"

#include <algorithm>

namespace Devilz::Scripting::Lua
{
bool Lua_Binding_Library_Manager::RegisterLibrary(std::unique_ptr<Lua_Binding_Library> library)
{
    if (!library || library->Name().empty() || FindLibrary(library->Name()))
        return false;

    m_libraries.push_back(std::move(library));
    return true;
}

bool Lua_Binding_Library_Manager::BindAll(Lua_Engine& engine, Lua_Commands& commands)
{
    if (!engine.Ready())
        return false;

    for (auto& library : m_libraries) {
        if (!library->Register(engine, commands))
            return false;
    }

    return true;
}

Lua_Binding_Library* Lua_Binding_Library_Manager::FindLibrary(std::string_view name) noexcept
{
    const auto it = std::find_if(
        m_libraries.begin(),
        m_libraries.end(),
        [name](const auto& library) { return library->Name() == name; });
    return it == m_libraries.end() ? nullptr : it->get();
}

const Lua_Binding_Library* Lua_Binding_Library_Manager::FindLibrary(std::string_view name) const noexcept
{
    const auto it = std::find_if(
        m_libraries.begin(),
        m_libraries.end(),
        [name](const auto& library) { return library->Name() == name; });
    return it == m_libraries.end() ? nullptr : it->get();
}

std::size_t Lua_Binding_Library_Manager::Count() const noexcept
{
    return m_libraries.size();
}

void Lua_Binding_Library_Manager::Clear() noexcept
{
    m_libraries.clear();
}
}
