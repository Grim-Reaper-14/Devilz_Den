#include "Lua_Page.hpp"

#include "Frontend/Menu/Themes/Menu_Theme.hpp"
#include "Scripting/Lua/Lua_Runtime.hpp"

#include <imgui.h>

#include <cstdint>
#include <string>
#include <utility>

namespace Devilz::Frontend::Lua
{
namespace
{
struct Lua_Page_State
{
    bool hasSelfTestResult{};
    bool selfTestSucceeded{};
    std::string selfTestMessage;
};

Lua_Page_State& PageState()
{
    static Lua_Page_State state;
    return state;
}

std::string ElementLabel(const std::string& label, std::uint64_t id)
{
    return label + "##lua_ui_" + std::to_string(id);
}
}

void DrawLuaPage()
{
    auto& runtime = Scripting::Lua::Lua_Runtime::Instance();
    const auto snapshot = runtime.Snapshot();

    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();
    ImGui::TextColored(palette.emberRed, "LUA");
    ImGui::SameLine();
    ImGui::TextDisabled("- embedded scripting runtime");
    Themes::Menu_Theme_Manager::Instance().DrawDivider();

    ImGui::TextColored(palette.bronze, "RUNTIME");
    const auto luaVersion = runtime.LuaVersion();
    const auto solVersion = runtime.Sol2Version();
    ImGui::Text("%.*s | Sol2 %.*s",
        static_cast<int>(luaVersion.size()), luaVersion.data(),
        static_cast<int>(solVersion.size()), solVersion.data());

    const ImVec4 statusColor = snapshot.ready
        ? ImVec4{0.42F, 0.78F, 0.42F, 1.0F}
        : ImVec4{0.92F, 0.30F, 0.24F, 1.0F};
    ImGui::TextColored(statusColor, "%s", snapshot.ready ? "READY" : "UNAVAILABLE");
    ImGui::SameLine();
    ImGui::TextDisabled("%s", snapshot.status.c_str());

    ImGui::TextDisabled(
        "Execution: %s",
        snapshot.dedicatedThread ? "Dedicated backend Lua thread" : "Not threaded");

    if (snapshot.ready) {
        const auto fingerprint = runtime.FingerprintHex();
        ImGui::TextDisabled("Fingerprint: %s", fingerprint.c_str());
        ImGui::TextDisabled(
            "Engines: %zu | Scripts: %zu | Modules: %zu | Libraries: %zu | Commands: %zu | Events: %zu",
            snapshot.engines,
            snapshot.scripts,
            snapshot.modules,
            snapshot.libraries,
            snapshot.commands,
            snapshot.events);
        ImGui::TextDisabled(
            "Settings: %zu | Features: %zu | UI: %zu | Scheduled tasks: %zu | Pending Lua jobs: %zu",
            snapshot.settings,
            snapshot.features,
            snapshot.uiElements,
            snapshot.scheduledTasks,
            snapshot.pendingJobs);

        ImGui::TextColored(palette.bronze, "HOT RELOAD");
        ImGui::TextDisabled(
            "%s | Scans: %zu | Reloads: %zu | Failures: %zu",
            snapshot.hotReloadEnabled ? "Enabled" : "Disabled",
            snapshot.hotReloadScans,
            snapshot.hotReloads,
            snapshot.hotReloadFailures);
        ImGui::TextDisabled("%s", snapshot.hotReloadStatus.c_str());
    }

    auto& pageState = PageState();
    ImGui::BeginDisabled(!snapshot.ready);
    if (ImGui::Button("RUN SOL2 SELF-TEST")) {
        auto result = runtime.RunSelfTest();
        pageState.hasSelfTestResult = true;
        pageState.selfTestSucceeded = result.succeeded;
        pageState.selfTestMessage = std::move(result.message);
    }
    ImGui::EndDisabled();

    if (pageState.hasSelfTestResult) {
        const ImVec4 resultColor = pageState.selfTestSucceeded
            ? ImVec4{0.42F, 0.78F, 0.42F, 1.0F}
            : ImVec4{0.92F, 0.30F, 0.24F, 1.0F};
        ImGui::TextColored(resultColor, "%s", pageState.selfTestMessage.c_str());
    }

    Themes::Menu_Theme_Manager::Instance().DrawDivider();
    ImGui::TextColored(palette.bronze, "SCRIPT UI");

    if (snapshot.ui.empty()) {
        ImGui::TextDisabled("No Lua script UI elements registered.");
    } else {
        for (const auto& element : snapshot.ui) {
            switch (element.type) {
            case Scripting::Lua::Lua_UI_Element_Type::Section:
                ImGui::Spacing();
                ImGui::TextColored(palette.emberRed, "%s", element.label.c_str());
                break;
            case Scripting::Lua::Lua_UI_Element_Type::Text:
                ImGui::TextWrapped("%s", element.label.c_str());
                break;
            case Scripting::Lua::Lua_UI_Element_Type::Button: {
                const auto label = ElementLabel(element.label, element.id);
                if (ImGui::Button(label.c_str()))
                    (void)runtime.SubmitUIActivate(element.id);
                break;
            }
            case Scripting::Lua::Lua_UI_Element_Type::Checkbox: {
                bool value = element.boolValue;
                const auto label = ElementLabel(element.label, element.id);
                if (ImGui::Checkbox(label.c_str(), &value))
                    (void)runtime.SubmitUICheckbox(element.id, value);
                break;
            }
            case Scripting::Lua::Lua_UI_Element_Type::SliderFloat: {
                float value = element.floatValue;
                const auto label = ElementLabel(element.label, element.id);
                if (ImGui::SliderFloat(
                        label.c_str(),
                        &value,
                        element.minValue,
                        element.maxValue)) {
                    (void)runtime.SubmitUISliderFloat(element.id, value);
                }
                break;
            }
            }
        }
    }

    Themes::Menu_Theme_Manager::Instance().DrawDivider();
    ImGui::TextColored(palette.bronze, "BINDINGS");
    ImGui::TextWrapped(
        "Bindings are split by domain. Core provides commands, fingerprint metadata, and cooperative tasks; Logger routes script messages into the runtime logger; Events provides owner-scoped callbacks; Settings provides typed script-owned values; Features provides controlled script-owned toggles; UI publishes declarative controls that marshal actions back to the dedicated Lua thread. Script content fingerprints drive automatic hot reload.");
}
}
