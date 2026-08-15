#include "Lua_Page.hpp"

#include "Frontend/Menu/Themes/Menu_Theme.hpp"
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
        ImGui::TextDisabled(
            "Engines: %zu | Scripts: %zu | Modules: %zu | Libraries: %zu | Commands: %zu | Events: %zu",
            snapshot.engines,
            snapshot.scripts,
            snapshot.modules,
            snapshot.libraries,
            snapshot.commands,
            snapshot.events);
        ImGui::TextDisabled(
            "Scheduled tasks: %zu | Pending Lua jobs: %zu",
            snapshot.scheduledTasks,
            snapshot.pendingJobs);
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
    ImGui::TextColored(palette.bronze, "BINDINGS");
    ImGui::TextWrapped(
        "Bindings are split by domain. Core provides commands and cooperative tasks, Logger routes script messages into the runtime logger, and Events provides script-owned subscriptions such as devilz.events.TICK.");
}
}
