#pragma once

#include "Lua_Module.hpp"

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>

namespace Devilz::Scripting::Lua
{
class Lua_Engine;

class Lua_Module_Manager final
{
public:
    std::size_t DiscoverModules(const std::filesystem::path& directory);
    bool RegisterModule(std::string name, std::filesystem::path path);
    bool LoadModule(Lua_Engine& engine, std::string_view name, std::string* error = nullptr) const;

    [[nodiscard]] Lua_Module* Find(std::string_view name) noexcept;
    [[nodiscard]] const Lua_Module* Find(std::string_view name) const noexcept;
    [[nodiscard]] std::size_t Count() const noexcept;

    void Clear() noexcept;

private:
    std::unordered_map<std::string, Lua_Module> m_modules;
};
}
