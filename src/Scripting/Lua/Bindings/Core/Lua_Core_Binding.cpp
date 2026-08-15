#include "Lua_Core_Binding.hpp"

#include "Scripting/Lua/Bindings/Lua_Binding_Context.hpp"
#include "Scripting/Lua/Lua_Binding_Library.hpp"
#include "Scripting/Lua/Lua_Commands.hpp"
#include "Scripting/Lua/Lua_Engine.hpp"
#include "Scripting/Lua/Lua_Scheduler.hpp"

#include <sol/sol.hpp>

#include <string>
#include <tuple>
#include <utility>

namespace Devilz::Scripting::Lua::Bindings::Core
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

    bool Register(Lua_Engine& engine, const Lua_Binding_Context& context) override
    {
        return RegisterCore(engine, context);
    }
};
}

bool RegisterCore(Lua_Engine& engine, const Lua_Binding_Context& context)
{
    if (!engine.Ready() || !context.commands)
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
    auto* commands = context.commands;

    commandTable.set_function(
        "register",
        [commands, ownerScriptId](std::string name, sol::protected_function callback) {
            return commands->Register(ownerScriptId, std::move(name), {}, std::move(callback));
        });

    commandTable.set_function(
        "execute",
        [commands](const std::string& name) {
            auto result = commands->Execute(name);
            return std::make_tuple(result.succeeded, std::move(result.message));
        });

    devilz["commands"] = commandTable;
    return Lua_Scheduler::Install(engine);
}

std::unique_ptr<Lua_Binding_Library> CreateCoreLibrary()
{
    return std::make_unique<Core_Library>();
}
}
