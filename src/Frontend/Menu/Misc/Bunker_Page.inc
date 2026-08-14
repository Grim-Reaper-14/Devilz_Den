// GTA Online Enhanced 1.73 Bunker business controls UI.
#include "Frontend/Menu/Themes/Menu_Theme.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Bunker_Extension.hpp"

#include <imgui.h>

#include <algorithm>

namespace Devilz::Frontend::Misc
{
namespace
{
using Integrations::GTA5_Enhanced::BunkerInstantSellSnapshot;
using Integrations::GTA5_Enhanced::BunkerTunablesActionSnapshot;
using Integrations::GTA5_Enhanced::BunkerTunablesSnapshot;
using Integrations::GTA5_Enhanced::GTA_Bunker_Instant_Sell_Status;
using Integrations::GTA5_Enhanced::GTA_Bunker_Instant_Sell_Status_Name;
using Integrations::GTA5_Enhanced::GTA_Bunker_Tunables_Action_Snapshot;
using Integrations::GTA5_Enhanced::GTA_Bunker_Tunables_Snapshot;
using Integrations::GTA5_Enhanced::GTA_Bunker_Tunables_Status;
using Integrations::GTA5_Enhanced::GTA_Bunker_Tunables_Status_Name;
using Integrations::GTA5_Enhanced::RequestBunkerInstantSell;
using Integrations::GTA5_Enhanced::RequestBunkerTunables;

struct BunkerTunablesEditor
{
    bool initialized = false;
    int productValue = 5000;
    float nearSaleMultiplier = 1.0F;
    float farSaleMultiplier = 1.5F;
    float highDemandBonus = 2.5F;
    float highDemandMaxBonus = 20.0F;

    void Load(const GTA_Bunker_Tunables_Snapshot& snapshot)
    {
        initialized = true;
        productValue = snapshot.productValue;
        nearSaleMultiplier = snapshot.nearSaleMultiplier;
        farSaleMultiplier = snapshot.farSaleMultiplier;
        highDemandBonus = snapshot.highDemandBonus;
        highDemandMaxBonus = snapshot.highDemandMaxBonus;
    }

    void LoadSuppliedValues()
    {
        initialized = true;
        productValue = 5000;
        nearSaleMultiplier = 1.0F;
        farSaleMultiplier = 1.5F;
        highDemandBonus = 2.5F;
        highDemandMaxBonus = 20.0F;
    }
};

BunkerTunablesEditor& BunkerEditor()
{
    static BunkerTunablesEditor editor;
    return editor;
}

void DrawBunkerTunablesActionStatus(const GTA_Bunker_Tunables_Action_Snapshot& action)
{
    if (action.status == GTA_Bunker_Tunables_Status::Idle)
        return;

    ImVec4 color{0.90F, 0.58F, 0.18F, 1.0F};
    if (action.status == GTA_Bunker_Tunables_Status::Succeeded)
        color = ImVec4{0.42F, 0.78F, 0.42F, 1.0F};
    else if (action.status != GTA_Bunker_Tunables_Status::Queued)
        color = ImVec4{0.92F, 0.32F, 0.28F, 1.0F};

    ImGui::TextColored(
        color,
        "[%s] Bunker Globals",
        GTA_Bunker_Tunables_Status_Name(action.status));
    if (!action.detail.empty())
        ImGui::TextWrapped("%s", action.detail.c_str());
    ImGui::Spacing();
}

void DrawBunkerInstantSellControls()
{
    auto& theme = Themes::Menu_Theme_Manager::Instance();
    const auto& palette = theme.Palette();
    const auto action = BunkerInstantSellSnapshot();
    const auto tunables = BunkerTunablesSnapshot();
    const auto tunablesAction = BunkerTunablesActionSnapshot();
    auto& editor = BunkerEditor();

    if (tunables.runtimeReady && !editor.initialized)
        editor.Load(tunables);

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

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextColored(palette.bronze, "BUNKER GLOBALS");
    ImGui::TextWrapped(
        "Live read/write controls for the Enhanced 1.73 Bunker product value, sale multipliers, and high-demand bonus tunables.");
    ImGui::TextDisabled("Product Value: Global_262145.f_21347");
    ImGui::TextDisabled("Near / Far: Global_262145.f_21319 / f_21320");
    ImGui::TextDisabled("High Demand: Global_262145.f_21232 / f_21233");
    ImGui::Spacing();

    DrawBunkerTunablesActionStatus(tunablesAction);

    if (tunables.runtimeReady) {
        ImGui::TextDisabled("LIVE VALUES");
        ImGui::Text(
            "Product $%d | Near %.2fx | Far %.2fx | Demand %.2fx | Max %.2f",
            tunables.productValue,
            tunables.nearSaleMultiplier,
            tunables.farSaleMultiplier,
            tunables.highDemandBonus,
            tunables.highDemandMaxBonus);
    } else {
        ImGui::TextWrapped("%s", tunables.detail.c_str());
    }

    ImGui::Spacing();
    if (tunables.runtimeReady &&
        ImGui::Button("RELOAD LIVE VALUES", ImVec2{190.0F, 30.0F})) {
        editor.Load(tunables);
    }

    ImGui::SameLine();
    if (ImGui::Button("LOAD SUPPLIED 1.73 VALUES", ImVec2{230.0F, 30.0F}))
        editor.LoadSuppliedValues();

    ImGui::Spacing();
    ImGui::SetNextItemWidth(220.0F);
    ImGui::InputInt("Product Value", &editor.productValue, 1000, 5000);
    ImGui::SetNextItemWidth(220.0F);
    ImGui::InputFloat("Near Sale Multiplier", &editor.nearSaleMultiplier, 0.1F, 1.0F, "%.2f");
    ImGui::SetNextItemWidth(220.0F);
    ImGui::InputFloat("Far Sale Multiplier", &editor.farSaleMultiplier, 0.1F, 1.0F, "%.2f");
    ImGui::SetNextItemWidth(220.0F);
    ImGui::InputFloat("High Demand Bonus", &editor.highDemandBonus, 0.1F, 1.0F, "%.2f");
    ImGui::SetNextItemWidth(220.0F);
    ImGui::InputFloat("High Demand Max Bonus", &editor.highDemandMaxBonus, 1.0F, 5.0F, "%.2f");

    const bool tunablesQueued =
        tunablesAction.status == GTA_Bunker_Tunables_Status::Queued;
    const bool tunablesDisabled = !tunables.runtimeReady || tunablesQueued;
    ImGui::BeginDisabled(tunablesDisabled);
    if (ImGui::Button("APPLY BUNKER GLOBALS", ImVec2{220.0F, 36.0F})) {
        editor.productValue = std::max(editor.productValue, 0);
        editor.nearSaleMultiplier = std::max(editor.nearSaleMultiplier, 0.0F);
        editor.farSaleMultiplier = std::max(editor.farSaleMultiplier, 0.0F);
        editor.highDemandBonus = std::max(editor.highDemandBonus, 0.0F);
        editor.highDemandMaxBonus = std::max(editor.highDemandMaxBonus, 0.0F);
        (void)RequestBunkerTunables(
            editor.productValue,
            editor.nearSaleMultiplier,
            editor.farSaleMultiplier,
            editor.highDemandBonus,
            editor.highDemandMaxBonus);
    }
    ImGui::EndDisabled();

    if (tunablesQueued) {
        ImGui::SameLine();
        ImGui::TextDisabled("Waiting for the GTA game thread...");
    }

    ImGui::Spacing();
    ImGui::TextDisabled(
        "Opening this page never changes the globals. Values are written only when APPLY BUNKER GLOBALS is pressed.");
}
}
}
