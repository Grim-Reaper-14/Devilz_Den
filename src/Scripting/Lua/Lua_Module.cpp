#include "Lua_Module.hpp"

#include <utility>

namespace Devilz::Scripting::Lua
{
Lua_Module::Lua_Module(std::string name, std::filesystem::path path)
    : m_name(std::move(name)), m_path(std::move(path))
{
}

std::string_view Lua_Module::Name() const noexcept
{
    return m_name;
}

const std::filesystem::path& Lua_Module::Path() const noexcept
{
    return m_path;
}
}
