#include "Lua_Module_Manager.hpp"

#include "Lua_Engine.hpp"

#include <sol/sol.hpp>

namespace Devilz::Scripting::Lua
{
std::size_t Lua_Module_Manager::DiscoverModules(const std::filesystem::path& directory)
{
    m_modules.clear();
    if (!std::filesystem::exists(directory))
        return 0;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".lua")
            continue;

        const auto name = entry.path().stem().string();
        RegisterModule(name, entry.path());
    }

    return m_modules.size();
}

bool Lua_Module_Manager::RegisterModule(std::string name, std::filesystem::path path)
{
    if (name.empty() || path.empty())
        return false;

    m_modules.insert_or_assign(name, Lua_Module{name, std::move(path)});
    return true;
}

bool Lua_Module_Manager::LoadModule(
    Lua_Engine& engine,
    std::string_view name,
    std::string* error) const
{
    const auto* module = Find(name);
    if (!module) {
        if (error)
            *error = "Lua module not found";
        return false;
    }

    if (!engine.Ready()) {
        if (error)
            *error = std::string{engine.Status()};
        return false;
    }

    auto result = engine.State().safe_script_file(module->Path().string(), sol::script_pass_on_error);
    if (!result.valid()) {
        const sol::error luaError = result;
        if (error)
            *error = luaError.what();
        return false;
    }

    return true;
}

Lua_Module* Lua_Module_Manager::Find(std::string_view name) noexcept
{
    const auto it = m_modules.find(std::string{name});
    return it == m_modules.end() ? nullptr : &it->second;
}

const Lua_Module* Lua_Module_Manager::Find(std::string_view name) const noexcept
{
    const auto it = m_modules.find(std::string{name});
    return it == m_modules.end() ? nullptr : &it->second;
}

std::size_t Lua_Module_Manager::Count() const noexcept
{
    return m_modules.size();
}

void Lua_Module_Manager::Clear() noexcept
{
    m_modules.clear();
}
}
