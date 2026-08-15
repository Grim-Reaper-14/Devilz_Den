#include "Lua_Settings_Binding.hpp"

#include "Scripting/Lua/Bindings/Lua_Binding_Context.hpp"
#include "Scripting/Lua/Lua_Binding_Library.hpp"
#include "Scripting/Lua/Lua_Engine.hpp"
#include "Scripting/Lua/Settings/Lua_Setting_Manager.hpp"

#include <sol/sol.hpp>

#include <cstdint>
#include <string>
#include <utility>

namespace Devilz::Scripting::Lua::Bindings::Settings
{
namespace
{
class Settings_Library final : public Lua_Binding_Library
{
public:
    [[nodiscard]] std::string_view Name() const noexcept override
    {
        return "settings";
    }

    bool Register(Lua_Engine& engine, const Lua_Binding_Context& context) override
    {
        return RegisterSettings(engine, context);
    }
};

bool FromLua(const sol::stack_object& object, Lua_Setting_Value& value)
{
    switch (object.get_type()) {
    case sol::type::boolean:
        value = object.as<bool>();
        return true;
    case sol::type::number: {
        auto* state = object.lua_state();
        const int index = object.stack_index();
        if (lua_isinteger(state, index) != 0) {
            value = static_cast<std::int64_t>(lua_tointeger(state, index));
        } else {
            value = static_cast<double>(lua_tonumber(state, index));
        }
        return true;
    }
    case sol::type::string:
        value = object.as<std::string>();
        return true;
    default:
        return false;
    }
}

sol::object ToLuaObject(sol::this_state luaState, const Lua_Setting_Value* value)
{
    sol::state_view lua(luaState);
    if (!value)
        return sol::make_object(lua, sol::nil);

    return std::visit(
        [&lua](const auto& current) {
            return sol::make_object(lua, current);
        },
        *value);
}
}

bool RegisterSettings(Lua_Engine& engine, const Lua_Binding_Context& context)
{
    if (!engine.Ready() || !context.settings)
        return false;

    auto& state = engine.State();
    const sol::object devilzObject = state["devilz"];
    if (!devilzObject.is<sol::table>())
        return false;

    auto devilz = devilzObject.as<sol::table>();
    auto settings = state.create_table();
    auto* manager = context.settings;
    const auto owner = engine.OwnerScriptId();

    settings.set_function(
        "register",
        [manager, owner](std::string name, sol::stack_object object) {
            Lua_Setting_Value value{false};
            if (!FromLua(object, value))
                return false;
            return manager->Register(owner, std::move(name), std::move(value));
        });

    settings.set_function(
        "get",
        [manager, owner](const std::string& name, sol::this_state luaState) {
            return ToLuaObject(luaState, manager->Get(owner, name));
        });

    settings.set_function(
        "set",
        [manager, owner](const std::string& name, sol::stack_object object) {
            Lua_Setting_Value value{false};
            if (!FromLua(object, value))
                return false;
            return manager->Set(owner, name, std::move(value));
        });

    settings.set_function(
        "reset",
        [manager, owner](const std::string& name) {
            return manager->Reset(owner, name);
        });

    settings.set_function(
        "unregister",
        [manager, owner](const std::string& name) {
            return manager->Unregister(owner, name);
        });

    settings.set_function(
        "exists",
        [manager, owner](const std::string& name) {
            return manager->Exists(owner, name);
        });

    settings.set_function(
        "type",
        [manager, owner](const std::string& name) {
            return std::string{Lua_Setting_Manager::TypeName(manager->Type(owner, name))};
        });

    settings.set_function(
        "count",
        [manager, owner]() {
            return manager->CountByOwner(owner);
        });

    devilz["settings"] = settings;
    return true;
}

std::unique_ptr<Lua_Binding_Library> CreateSettingsLibrary()
{
    return std::make_unique<Settings_Library>();
}
}
