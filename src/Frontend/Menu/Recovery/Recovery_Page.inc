#include "Frontend/Menu/Themes/Menu_Theme.hpp"

#include <imgui.h>

namespace Devilz::Frontend::Recovery
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

void DrawRecoveryPage()
{
    auto& theme = Themes::Menu_Theme_Manager::Instance();
    const auto& palette = theme.Palette();

    ImGui::TextColored(palette.emberRed, "RECOVERY");
    ImGui::SameLine();
    ImGui::TextDisabled("- progression and unlocks");

    DrawDivider();

    ImGui::TextColored(palette.bronze, "PAGE STATUS");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4{0.90F, 0.58F, 0.18F, 1.0F}, "[UI ONLY]");

    ImGui::TextWrapped(
        "Recovery is reserved for progression, unlock and collection status views. "
        "Controls remain disabled until their supporting runtime is added.");
    ImGui::Spacing();

    if (ImGui::BeginTabBar("##RecoveryPageTabs")) {
        if (ImGui::BeginTabItem("PROGRESSION")) {
            ImGui::TextColored(palette.bronze, "PROGRESSION");
            DrawDivider();
            DrawDisabledRow("Progression Status", "Reserved status view for future progression integrations.");
            DrawDisabledRow("Progression Summary", "Reserved summary view for future progression integrations.");
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("UNLOCKS")) {
            ImGui::TextColored(palette.bronze, "UNLOCKS");
            DrawDivider();
            DrawDisabledRow("Unlock Status", "Reserved status view for future unlock integrations.");
            DrawDisabledRow("Unlock Summary", "Reserved summary view for future unlock integrations.");
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("COLLECTIONS")) {
            ImGui::TextColored(palette.bronze, "COLLECTIONS");
            DrawDivider();
            DrawDisabledRow("Collection Status", "Reserved status view for future collection integrations.");
            DrawDisabledRow("Collection Summary", "Reserved summary view for future collection integrations.");
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("ACTIVITY")) {
            ImGui::TextColored(palette.bronze, "RECOVERY ACTIVITY");
            DrawDivider();
            ImGui::TextDisabled("No activity recorded.");
            ImGui::TextDisabled("Activity will appear here when the Recovery runtime is connected.");
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}
}
