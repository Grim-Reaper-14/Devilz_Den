#include "Lua_Features_Binding.hpp"

#include "Scripting/Lua/Bindings/Lua_Binding_Context.hpp"
#include "Scripting/Lua/Features/Lua_Feature_Manager.hpp"
#include "Scripting/Lua/Lua_Binding_Library.hpp"
#include "Scripting/Lua/Lua_Engine.hpp"

#include <sol/sol.hpp>

#include <string>
#include <tuple>
#include <utility>

namespace Devilz::Scripting::Lua::Bindings::Features
{
namespace
{
class Features_Library final : public Lua_Binding_Library
{
public:
    [[nodiscard]] std::string_view Name() const noexcept override
    {
        return "features";
    }

    bool Register(Lua_Engine& engine, const Lua_Binding_Context& context) override
    {
        return RegisterFeatures(engine, context);
    }
};
}

bool RegisterFeatures(Lua_Engine& engine, const Lua_Binding_Context& context)
{
    if (!engine.Ready() || !context.features)
        return false;

    auto& state = engine.State();
    const sol::object devilzObject = state["devilz"];
    if (!devilzObject.is<sol::table>())
        return false;

    auto devilz = devilzObject.as<sol::table>();
    auto features = state.create_table();
    auto* manager = context.features;
    const auto owner = engine.OwnerScriptId();

    features.set_function(
        "register",
        sol::overload(
            [manager, owner](std::string name) {
                return manager->Register(owner, std::move(name), false);
            },
            [manager, owner](std::string name, bool enabled) {
                return manager->Register(owner, std::move(name), enabled);
            }));

    features.set_function(
        "available",
        [manager, owner](const std::string& name) {
            return manager->Available(owner, name);
        });

    features.set_function(
        "enabled",
        [manager, owner](const std::string& name) {
            return manager->Enabled(owner, name);
        });

    features.set_function(
        "set",
        [manager, owner](const std::string& name, bool enabled) {
            return manager->Set(owner, name, enabled);
        });

    features.set_function(
        "toggle",
        [manager, owner](const std::string& name) {
            bool enabled = false;
            const bool succeeded = manager->Toggle(owner, name, &enabled);
            return std::make_tuple(succeeded, enabled);
        });

    features.set_function(
        "reset",
        [manager, owner](const std::string& name) {
            return manager->Reset(owner, name);
        });

    features.set_function(
        "unregister",
        [manager, owner](const std::string& name) {
            return manager->Unregister(owner, name);
        });

    features.set_function(
        "count",
        [manager, owner]() {
            return manager->CountByOwner(owner);
        });

    devilz["features"] = features;
    return true;
}

std::unique_ptr<Lua_Binding_Library> CreateFeaturesLibrary()
{
    return std::make_unique<Features_Library>();
}
}
