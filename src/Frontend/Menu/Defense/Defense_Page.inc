#include "Defense_Page.hpp"

#include "Frontend/Menu/Themes/Menu_Theme.hpp"

#include <imgui.h>

namespace Devilz::Frontend::Defense
{
namespace
{
void DrawPendingOption(const char* label, const char* description)
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

void DrawSectionDivider()
{
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}

void DrawPlayerProtection()
{
    auto& theme = Themes::Menu_Theme_Manager::Instance();
    const auto& palette = theme.Palette();

    ImGui::TextColored(palette.bronze, "PLAYER PROTECTION");
    DrawSectionDivider();

    DrawPendingOption(
        "Player Event Validation",
        "Reserved defensive validation for incoming player-related activity.");

    DrawPendingOption(
        "Local Player Guard",
        "Reserved protection policy for the local player.");

    DrawPendingOption(
        "Vehicle State Guard",
        "Reserved protection policy for the currently occupied vehicle.");

    DrawPendingOption(
        "Position Guard",
        "Reserved policy for unexpected changes to the local player's position.");

    DrawPendingOption(
        "Removal Guard",
        "Reserved protection policy for local player and controlled vehicle state.");
}

void DrawScriptProtection()
{
    auto& theme = Themes::Menu_Theme_Manager::Instance();
    const auto& palette = theme.Palette();

    ImGui::TextColored(palette.bronze, "SCRIPT PROTECTION");
    DrawSectionDivider();

    DrawPendingOption(
        "Script Event Validation",
        "Reserved validation policy for incoming script-related activity.");

    DrawPendingOption(
        "Action Guard",
        "Reserved policy for unexpected local actions.");

    DrawPendingOption(
        "Payload Validation",
        "Reserved structural validation for Defense runtime input.");

    DrawPendingOption(
        "Session State Guard",
        "Reserved defensive policy for unexpected session-state activity.");
}

void DrawDisturbanceProtection()
{
    auto& theme = Themes::Menu_Theme_Manager::Instance();
    const auto& palette = theme.Palette();

    ImGui::TextColored(palette.bronze, "DISTURBANCE PROTECTION");
    DrawSectionDivider();

    DrawPendingOption(
        "Sound Guard",
        "Reserved defensive policy for disruptive sound activity.");

    DrawPendingOption(
        "Camera Guard",
        "Reserved defensive policy for unexpected camera disturbance.");

    DrawPendingOption(
        "Player State Guard",
        "Reserved policy for disruptive local-player state changes.");
}

void DrawActivity()
{
    auto& theme = Themes::Menu_Theme_Manager::Instance();
    const auto& palette = theme.Palette();

    ImGui::TextColored(palette.bronze, "DEFENSE ACTIVITY");
    DrawSectionDivider();

    ImGui::TextDisabled("Events handled");
    ImGui::SameLine(180.0F);
    ImGui::TextColored(palette.parchment, "0");

    ImGui::TextDisabled("Warnings");
    ImGui::SameLine(180.0F);
    ImGui::TextColored(palette.parchment, "0");

    ImGui::TextDisabled("Last source");
    ImGui::SameLine(180.0F);
    ImGui::TextColored(palette.parchment, "None");

    DrawSectionDivider();

    constexpr ImGuiTableFlags flags =
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_SizingStretchProp;

    if (ImGui::BeginTable("##DefenseActivityTable", 4, flags)) {
        ImGui::TableSetupColumn("SOURCE");
        ImGui::TableSetupColumn("CATEGORY");
        ImGui::TableSetupColumn("STATUS");
        ImGui::TableSetupColumn("COUNT");
        ImGui::TableHeadersRow();

        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::TextDisabled("-");

        ImGui::TableSetColumnIndex(1);
        ImGui::TextDisabled("No activity");

        ImGui::TableSetColumnIndex(2);
        ImGui::TextDisabled("Runtime pending");

        ImGui::TableSetColumnIndex(3);
        ImGui::TextDisabled("0");

        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::TextDisabled(
        "Defense activity will appear here once a runtime source is connected.");
}
}

void DrawDefensePage()
{
    auto& theme = Themes::Menu_Theme_Manager::Instance();
    const auto& palette = theme.Palette();

    ImGui::TextColored(palette.emberRed, "DEFENSE");
    ImGui::SameLine();
    ImGui::TextDisabled("- local safety and defensive status");

    DrawSectionDivider();

    ImGui::TextColored(palette.bronze, "DEFENSE STATUS");
    ImGui::SameLine();
    ImGui::TextColored(
        ImVec4{0.90F, 0.28F, 0.18F, 1.0F},
        "[RUNTIME NOT CONNECTED]");

    ImGui::Spacing();
    ImGui::TextWrapped(
        "Defense centralizes local safety policies, runtime status and activity information. "
        "Options remain unavailable until their corresponding runtime support is connected.");

    ImGui::Spacing();

    if (ImGui::BeginTabBar("##DefensePageTabs")) {
        if (ImGui::BeginTabItem("PLAYER")) {
            DrawPlayerProtection();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("SCRIPT")) {
            DrawScriptProtection();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("DISTURBANCE")) {
            DrawDisturbanceProtection();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("ACTIVITY")) {
            DrawActivity();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}
}
