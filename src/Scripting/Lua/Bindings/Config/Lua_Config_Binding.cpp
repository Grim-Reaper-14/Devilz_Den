#include "Lua_Config_Binding.hpp"

#include "Scripting/Lua/Config/Lua_Config_Manager.hpp"
#include "Scripting/Lua/Lua_Engine.hpp"
#include "Scripting/Lua/Settings/Lua_Setting_Manager.hpp"

#include <sol/sol.hpp>

#include <string>
#include <tuple>

namespace Devilz::Scripting::Lua::Bindings::Config
{
bool RegisterConfig(
    Lua_Engine& engine,
    Lua_Config_Manager& configs,
    Lua_Setting_Manager& settings)
{
    if (!engine.Ready())
        return false;

    auto& state = engine.State();
    const sol::object devilzObject = state["devilz"];
    if (!devilzObject.is<sol::table>())
        return false;

    auto devilz = devilzObject.as<sol::table>();
    auto config = state.create_table();
    const auto owner = engine.OwnerScriptId();

    config["available"] = configs.Available(owner);
    config["script_key"] = configs.ScriptKey(owner);

    config.set_function(
        "save",
        [&configs, &settings, owner](const std::string& profile) {
            const auto result = configs.Save(owner, profile, settings);
            return std::make_tuple(result.succeeded, result.applied, result.message);
        });

    config.set_function(
        "load",
        [&configs, &settings, owner](const std::string& profile) {
            const auto result = configs.Load(owner, profile, settings);
            return std::make_tuple(result.succeeded, result.applied, result.message);
        });

    config.set_function(
        "remove",
        [&configs, owner](const std::string& profile) {
            const auto result = configs.Remove(owner, profile);
            return std::make_tuple(result.succeeded, result.message);
        });

    config.set_function(
        "list",
        [&configs, owner](sol::this_state luaState) {
            sol::state_view lua(luaState);
            const auto profiles = configs.List(owner);
            auto result = lua.create_table(static_cast<int>(profiles.size()), 0);
            for (std::size_t index = 0; index < profiles.size(); ++index)
                result[index + 1U] = profiles[index];
            return result;
        });

    devilz["config"] = config;
    return true;
}
}
