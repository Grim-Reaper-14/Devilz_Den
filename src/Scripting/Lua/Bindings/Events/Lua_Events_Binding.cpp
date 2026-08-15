#include "Lua_Events_Binding.hpp"

#include "Scripting/Lua/Bindings/Lua_Binding_Context.hpp"
#include "Scripting/Lua/Events/Lua_Event_Manager.hpp"
#include "Scripting/Lua/Lua_Binding_Library.hpp"
#include "Scripting/Lua/Lua_Engine.hpp"

#include <sol/sol.hpp>

#include <string>
#include <utility>

namespace Devilz::Scripting::Lua::Bindings::Events
{
namespace
{
class Events_Library final : public Lua_Binding_Library
{
public:
    [[nodiscard]] std::string_view Name() const noexcept override
    {
        return "events";
    }

    bool Register(Lua_Engine& engine, const Lua_Binding_Context& context) override
    {
        return RegisterEvents(engine, context);
    }
};
}

bool RegisterEvents(Lua_Engine& engine, const Lua_Binding_Context& context)
{
    if (!engine.Ready() || !context.events)
        return false;

    auto& state = engine.State();
    const sol::object devilzObject = state["devilz"];
    if (!devilzObject.is<sol::table>())
        return false;

    auto devilz = devilzObject.as<sol::table>();
    auto events = state.create_table();
    auto* eventManager = context.events;
    const auto ownerScriptId = engine.OwnerScriptId();

    events["TICK"] = "tick";

    events.set_function(
        "on",
        [eventManager, ownerScriptId](std::string name, sol::protected_function callback) {
            return eventManager->Subscribe(
                ownerScriptId,
                std::move(name),
                std::move(callback));
        });

    events.set_function(
        "off",
        [eventManager, ownerScriptId](Lua_Event_Manager::Subscription_Id id) {
            return eventManager->Unsubscribe(ownerScriptId, id);
        });

    events.set_function(
        "count",
        [eventManager, ownerScriptId]() {
            return eventManager->CountByOwner(ownerScriptId);
        });

    devilz["events"] = events;
    return true;
}

std::unique_ptr<Lua_Binding_Library> CreateEventsLibrary()
{
    return std::make_unique<Events_Library>();
}
}
