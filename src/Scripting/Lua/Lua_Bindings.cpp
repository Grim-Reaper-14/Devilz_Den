#include "Lua_Bindings.hpp"

#include "Lua_Binding_Library.hpp"
#include "Lua_Commands.hpp"
#include "Lua_Engine.hpp"

#include <string>
#include <tuple>
#include <utility>

namespace Devilz::Scripting::Lua::Lua_Bindings
{
namespace
{
class Core_Library final : public Lua_Binding_Library
{
public:
    [[nodiscard]] std::string_view Name() const noexcept override
    {
        return "core";
    }

    bool Register(Lua_Engine& engine, Lua_Commands& commands) override
    {
        return RegisterCore(engine, commands);
    }
};
}

bool RegisterCore(Lua_Engine& engine, Lua_Commands& commands)
{
    if (!engine.Ready())
        return false;

    auto& state = engine.State();
    auto devilz = state.create_named_table("devilz");
    devilz["api_version"] = 1;
    devilz["runtime"] = "Devilz Den";
    devilz["engine_id"] = engine.GetId();
    devilz["owner_script_id"] = engine.OwnerScriptId();

    devilz.set_function("version", []() {
        return std::string{LUA_VERSION};
    });

    auto commandTable = state.create_table();
    const auto ownerScriptId = engine.OwnerScriptId();

    commandTable.set_function(
        "register",
        [&commands, ownerScriptId](std::string name, sol::protected_function callback) {
            return commands.Register(ownerScriptId, std::move(name), {}, std::move(callback));
        });

    commandTable.set_function(
        "execute",
        [&commands](const std::string& name) {
            auto result = commands.Execute(name);
            return std::make_tuple(result.succeeded, std::move(result.message));
        });

    devilz["commands"] = commandTable;
    return true;
}

std::unique_ptr<Lua_Binding_Library> CreateCoreLibrary()
{
    return std::make_unique<Core_Library>();
}
}
