#include "Lua_Page.hpp"

#include "Frontend/Menu/Themes/Menu_Theme.hpp"
#include "Scripting/Lua/Lua_Manager.hpp"
#include "Scripting/Lua/Lua_Runtime.hpp"

#include <imgui.h>

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
}

void DrawLuaPage()
{
    auto& runtime = Scripting::Lua::Lua_Runtime::Instance();
    runtime.Initialize();
    auto& manager = Scripting::Lua::Lua_Manager::Instance();

    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();
    ImGui::TextColored(palette.emberRed, "LUA");
    ImGui::SameLine();
    ImGui::TextDisabled("- embedded scripting runtime");
    Themes::Menu_Theme_Manager::Instance().DrawDivider();

    ImGui::TextColored(palette.bronze, "RUNTIME");
    ImGui::Text("%.*s | Sol2 %.*s",
        static_cast<int>(runtime.LuaVersion().size()), runtime.LuaVersion().data(),
        static_cast<int>(runtime.Sol2Version().size()), runtime.Sol2Version().data());

    const ImVec4 statusColor = runtime.Ready()
        ? ImVec4{0.42F, 0.78F, 0.42F, 1.0F}
        : ImVec4{0.92F, 0.30F, 0.24F, 1.0F};
    ImGui::TextColored(statusColor, "%s", runtime.Ready() ? "READY" : "UNAVAILABLE");
    ImGui::SameLine();
    ImGui::TextDisabled("%.*s", static_cast<int>(runtime.Status().size()), runtime.Status().data());

    if (runtime.Ready()) {
        ImGui::TextDisabled(
            "Engines: %zu | Scripts: %zu | Modules: %zu | Libraries: %zu | Commands: %zu",
            manager.Engines().Count(),
            manager.Scripts().Scripts().size(),
            manager.Modules().Count(),
            manager.Libraries().Count(),
            manager.Commands().Count());
    }

    auto& pageState = PageState();
    ImGui::BeginDisabled(!runtime.Ready());
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
    ImGui::TextColored(palette.bronze, "BINDINGS");
    ImGui::TextWrapped(
        "Core bindings are active. Scripts can use runtime metadata and the script-owned devilz.commands API. Game bindings are still gated for later libraries.");
}
}
