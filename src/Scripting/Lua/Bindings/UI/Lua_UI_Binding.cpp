#include "Lua_UI_Binding.hpp"

#include "Scripting/Lua/Bindings/Lua_Binding_Context.hpp"
#include "Scripting/Lua/Lua_Binding_Library.hpp"
#include "Scripting/Lua/Lua_Engine.hpp"
#include "Scripting/Lua/UI/Lua_UI_Manager.hpp"

#include <sol/sol.hpp>

#include <string>
#include <utility>

namespace Devilz::Scripting::Lua::Bindings::UI
{
namespace
{
class UI_Library final : public Lua_Binding_Library
{
public:
    [[nodiscard]] std::string_view Name() const noexcept override
    {
        return "ui";
    }

    bool Register(Lua_Engine& engine, const Lua_Binding_Context& context) override
    {
        return RegisterUI(engine, context);
    }
};
}

bool RegisterUI(Lua_Engine& engine, const Lua_Binding_Context& context)
{
    if (!engine.Ready() || !context.ui)
        return false;

    auto& state = engine.State();
    const sol::object devilzObject = state["devilz"];
    if (!devilzObject.is<sol::table>())
        return false;

    auto devilz = devilzObject.as<sol::table>();
    auto ui = state.create_table();
    auto* manager = context.ui;
    const auto owner = engine.OwnerScriptId();

    ui.set_function(
        "section",
        [manager, owner](std::string label) {
            return manager->AddSection(owner, std::move(label));
        });

    ui.set_function(
        "text",
        [manager, owner](std::string text) {
            return manager->AddText(owner, std::move(text));
        });

    ui.set_function(
        "button",
        [manager, owner](std::string label, sol::protected_function callback) {
            return manager->AddButton(
                owner,
                std::move(label),
                std::move(callback));
        });

    ui.set_function(
        "checkbox",
        [manager, owner](
            std::string label,
            bool value,
            sol::protected_function callback) {
            return manager->AddCheckbox(
                owner,
                std::move(label),
                value,
                std::move(callback));
        });

    ui.set_function(
        "slider_float",
        [manager, owner](
            std::string label,
            float value,
            float minValue,
            float maxValue,
            sol::protected_function callback) {
            return manager->AddSliderFloat(
                owner,
                std::move(label),
                value,
                minValue,
                maxValue,
                std::move(callback));
        });

    ui.set_function(
        "remove",
        [manager, owner](Lua_UI_Manager::Element_Id id) {
            return manager->Remove(owner, id);
        });

    ui.set_function(
        "count",
        [manager, owner]() {
            return manager->CountByOwner(owner);
        });

    devilz["ui"] = ui;
    return true;
}

std::unique_ptr<Lua_Binding_Library> CreateUILibrary()
{
    return std::make_unique<UI_Library>();
}
}
