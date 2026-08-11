#include "Lua_Page.hpp"

#include "Frontend/Menu/Themes/Menu_Theme.hpp"

#include <imgui.h>

namespace Devilz::Frontend::Lua
{
void DrawLuaPage()
{
    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();
    ImGui::TextColored(palette.emberRed, "LUA");
    ImGui::SameLine();
    ImGui::TextDisabled("- future embedded scripting");
    Themes::Menu_Theme_Manager::Instance().DrawDivider();

    ImGui::TextColored(palette.bronze, "SCRIPT MANAGER");
    ImGui::TextWrapped("Reserved for load, reload, unload, script errors, per-script settings, and coroutine/tick support.");

    Themes::Menu_Theme_Manager::Instance().DrawDivider();
    ImGui::TextColored(palette.bronze, "BINDINGS");
    ImGui::TextWrapped("Lua bindings will call the same Self, Weapons, Vehicle, Teleport, World, and Network backend APIs used by the native menu.");
}
}
