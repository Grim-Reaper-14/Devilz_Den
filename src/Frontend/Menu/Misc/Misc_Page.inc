#include "Misc_Page.hpp"

#include "Frontend/Menu/Themes/Menu_Theme.hpp"

#include <imgui.h>

namespace Devilz::Frontend::Misc
{
namespace
{
void DrawDivider()
{
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}

void DrawDisabledRow(const char* label, const char* description)
{
    bool enabled = false;
    ImGui::BeginDisabled(true);
    ImGui::Checkbox(label, &enabled);
    ImGui::EndDisabled();
    ImGui::Indent();
    ImGui::TextDisabled("%s", description);
    ImGui::Unindent();
    ImGui::Spacing();
}
}

void DrawMiscPage()
{
    auto& theme = Themes::Menu_Theme_Manager::Instance();
    const auto& palette = theme.Palette();

    ImGui::TextColored(palette.emberRed, "MISC");
    ImGui::SameLine();
    ImGui::TextDisabled("- business tools");

    DrawDivider();

    ImGui::TextColored(palette.bronze, "PAGE STATUS");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4{0.90F, 0.58F, 0.18F, 1.0F}, "[UI ONLY]");

    ImGui::TextWrapped(
        "Misc is reserved for business-related tools and status views. "
        "Controls remain disabled until their supporting runtime is added.");
    ImGui::Spacing();

    if (ImGui::BeginTabBar("##MiscPageTabs")) {
        if (ImGui::BeginTabItem("OVERVIEW")) {
            ImGui::TextColored(palette.bronze, "BUSINESS OVERVIEW");
            DrawDivider();
            DrawDisabledRow("Business Status", "Reserved status view for future business integrations.");
            DrawDisabledRow("Business Summary", "Reserved summary view for future business integrations.");
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("TOOLS")) {
            ImGui::TextColored(palette.bronze, "BUSINESS TOOLS");
            DrawDivider();
            DrawDisabledRow("Business Tools", "Reserved controls for the future Misc runtime.");
            DrawDisabledRow("Business Utilities", "Reserved utilities for the future Misc runtime.");
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("ACTIVITY")) {
            ImGui::TextColored(palette.bronze, "BUSINESS ACTIVITY");
            DrawDivider();
            ImGui::TextDisabled("No activity recorded.");
            ImGui::TextDisabled("Activity will appear here when the Misc runtime is connected.");
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}
}
