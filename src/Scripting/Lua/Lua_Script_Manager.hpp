#pragma once

#include "Lua_Script.hpp"

#include <filesystem>
#include <memory>
#include <vector>

namespace Devilz::Scripting::Lua
{
class Lua_Binding_Library_Manager;
class Lua_Commands;
class Lua_Engine_Manager;

class Lua_Script_Manager final
{
public:
    void Configure(
        Lua_Engine_Manager* engines,
        Lua_Binding_Library_Manager* libraries,
        Lua_Commands* commands) noexcept;

    std::size_t DiscoverScripts(const std::filesystem::path& directory);
    Lua_Script* LoadScript(const std::filesystem::path& path);
    bool UnloadScript(Lua_Script::Id id) noexcept;
    bool ReloadScript(Lua_Script::Id id);

    void Tick();
    void UnloadAll() noexcept;

    [[nodiscard]] Lua_Script* FindScript(Lua_Script::Id id) noexcept;
    [[nodiscard]] const std::vector<std::unique_ptr<Lua_Script>>& Scripts() const noexcept;
    [[nodiscard]] const std::vector<std::filesystem::path>& DiscoveredScripts() const noexcept;

private:
    Lua_Script::Id m_nextId{1};
    Lua_Engine_Manager* m_engines{};
    Lua_Binding_Library_Manager* m_libraries{};
    Lua_Commands* m_commands{};
    std::vector<std::unique_ptr<Lua_Script>> m_scripts;
    std::vector<std::filesystem::path> m_discoveredScripts;
};
}
