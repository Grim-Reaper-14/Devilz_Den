#include "Frontend/Menu/Themes/Menu_Theme.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Bunker_Extension.hpp"

#include <imgui.h>

namespace Devilz::Frontend::Misc
{
namespace
{
using Integrations::GTA5_Enhanced::BunkerInstantSellSnapshot;
using Integrations::GTA5_Enhanced::GTA_Bunker_Instant_Sell_Status;
using Integrations::GTA5_Enhanced::GTA_Bunker_Instant_Sell_Status_Name;
using Integrations::GTA5_Enhanced::RequestBunkerInstantSell;

void DrawBunkerInstantSellControls()
{
    auto& theme = Themes::Menu_Theme_Manager::Instance();
    const auto& palette = theme.Palette();
    const auto action = BunkerInstantSellSnapshot();

    ImGui::TextColored(palette.bronze, "BUNKER INSTANT-SELL");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextWrapped(
        "Enhanced 1.73 / b1158.13 only. Start a Bunker sell mission first; Devil's Den "
        "then resolves the active gb_gunrunning script and completes the mission with a one-shot local write.");
    ImGui::TextDisabled("gb_gunrunning local: 1275 + 774 = 2049 -> 0");
    ImGui::TextDisabled(
        "The write runs on the GTA game thread, validates the script stack, and verifies the value by readback.");
    ImGui::Spacing();

    if (action.status != GTA_Bunker_Instant_Sell_Status::Idle) {
        ImVec4 color{0.90F, 0.58F, 0.18F, 1.0F};
        if (action.status == GTA_Bunker_Instant_Sell_Status::Succeeded)
            color = ImVec4{0.42F, 0.78F, 0.42F, 1.0F};
        else if (action.status != GTA_Bunker_Instant_Sell_Status::Queued)
            color = ImVec4{0.92F, 0.32F, 0.28F, 1.0F};

        ImGui::TextColored(
            color,
            "[%s] Bunker Instant-Sell",
            GTA_Bunker_Instant_Sell_Status_Name(action.status));
        if (!action.detail.empty())
            ImGui::TextWrapped("%s", action.detail.c_str());
        ImGui::Spacing();
    }

    const bool queued = action.status == GTA_Bunker_Instant_Sell_Status::Queued;
    ImGui::BeginDisabled(queued);
    if (ImGui::Button("COMPLETE BUNKER SELL", ImVec2{220.0F, 36.0F}))
        (void)RequestBunkerInstantSell();
    ImGui::EndDisabled();

    if (queued) {
        ImGui::SameLine();
        ImGui::TextDisabled("Waiting for the GTA game thread...");
    }

    ImGui::Spacing();
    ImGui::TextDisabled(
        "If gb_gunrunning is not active, the action fails closed without writing another script's stack.");
}
}
}
