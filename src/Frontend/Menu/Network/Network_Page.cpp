#include "Network_Page.hpp"

#include "Frontend/Menu/Themes/Menu_Theme.hpp"

#include <imgui.h>

namespace Devilz::Frontend::Network
{
void DrawNetworkPage()
{
    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();
    ImGui::TextColored(palette.emberRed, "NETWORK");
    ImGui::SameLine();
    ImGui::TextDisabled("- sessions, players and protections");
    Themes::Menu_Theme_Manager::Instance().DrawDivider();

    ImGui::TextColored(palette.bronze, "SESSIONS");
    ImGui::TextWrapped("Session browsing and session-state tools will live in Frontend/Menu/Network/Sessions, backed by the GTA Enhanced Network runtime layer.");

    Themes::Menu_Theme_Manager::Instance().DrawDivider();
    ImGui::TextColored(palette.bronze, "PLAYERS");
    ImGui::TextDisabled("Player list module reserved for the Network/Players domain.");

    Themes::Menu_Theme_Manager::Instance().DrawDivider();
    ImGui::TextColored(palette.bronze, "PROTECTIONS");
    ImGui::TextDisabled("Network protections will be kept separate from session/player presentation code.");
}
}
