#include "Misc_Page.hpp"

#include "Frontend/Menu/Themes/Menu_Theme.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Business_State.hpp"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace Devilz::Frontend::Misc
{
namespace
{
using Integrations::GTA5_Enhanced::GTA_Business_State;
using Integrations::GTA5_Enhanced::GTA_Nightclub_Action_Name;
using Integrations::GTA5_Enhanced::GTA_Nightclub_Action_Snapshot;
using Integrations::GTA5_Enhanced::GTA_Nightclub_Action_Status;
using Integrations::GTA5_Enhanced::GTA_Nightclub_Action_Status_Name;
using Integrations::GTA5_Enhanced::GTA_Nightclub_Good;
using Integrations::GTA5_Enhanced::GTA_Nightclub_Good_Count;
using Integrations::GTA5_Enhanced::GTA_Nightclub_Good_Name;
using Integrations::GTA5_Enhanced::GTA_Nightclub_Safe_Capacity;
using Integrations::GTA5_Enhanced::GTA_Nightclub_Sale_Values;
using Integrations::GTA5_Enhanced::GTA_Nightclub_Snapshot;
using Integrations::GTA5_Enhanced::GTA_Resupply_Action_Kind;
using Integrations::GTA5_Enhanced::GTA_Resupply_Action_Name;
using Integrations::GTA5_Enhanced::GTA_Resupply_Action_Snapshot;
using Integrations::GTA5_Enhanced::GTA_Resupply_Action_Status;
using Integrations::GTA5_Enhanced::GTA_Resupply_Action_Status_Name;
using Integrations::GTA5_Enhanced::GTA_Resupply_Target;
using Integrations::GTA5_Enhanced::GTA_Resupply_Target_Count;
using Integrations::GTA5_Enhanced::GTA_Resupply_Target_Name;

struct NightclubEditorState
{
    bool initialized = false;
    int nightclubIndex = -1;
    int businessHubIndex = -1;
    float popularityPercent = 0.0F;
    int safeCash = 0;
    int entryCost = 0;
    std::array<int, GTA_Nightclub_Good_Count> productStocks{};
    GTA_Nightclub_Sale_Values sale{};
    int missionIndex = -1;
    int defendMissionIndex = -1;

    void Load(const GTA_Nightclub_Snapshot& snapshot)
    {
        initialized = true;
        nightclubIndex = snapshot.nightclubIndex;
        businessHubIndex = snapshot.businessHubIndex;
        popularityPercent = std::isfinite(snapshot.popularity)
            ? std::clamp(snapshot.popularity * 100.0F, 0.0F, 100.0F)
            : 0.0F;
        safeCash = snapshot.safeCash;
        entryCost = snapshot.entryCost;
        sale = snapshot.sale.values;
        missionIndex = snapshot.missionIndex;
        defendMissionIndex = snapshot.defendMissionIndex;

        for (std::size_t index = 0; index < productStocks.size(); ++index)
            productStocks[index] = snapshot.goods[index].units;
    }
};

NightclubEditorState& EditorState()
{
    static NightclubEditorState state;
    return state;
}

void DrawDivider()
{
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}

void DrawStatusValue(const char* label, const char* value)
{
    ImGui::TextDisabled("%s", label);
    ImGui::SameLine(190.0F);
    ImGui::TextUnformatted(value);
}

void DrawCurrencyValue(const char* label, int value)
{
    ImGui::TextDisabled("%s", label);
    ImGui::SameLine(190.0F);
    ImGui::Text("$%d", value);
}

void DrawOverview(const GTA_Nightclub_Snapshot& nightclub)
{
    auto& theme = Themes::Menu_Theme_Manager::Instance();
    const auto& palette = theme.Palette();

    ImGui::TextColored(palette.bronze, "BUSINESS OVERVIEW");
    DrawDivider();

    ImGui::TextDisabled("Nightclub Runtime");
    ImGui::SameLine(190.0F);
    if (nightclub.runtimeReady)
        ImGui::TextColored(ImVec4{0.42F, 0.78F, 0.42F, 1.0F}, "READY");
    else
        ImGui::TextColored(ImVec4{0.90F, 0.58F, 0.18F, 1.0F}, "WAITING");

    if (!nightclub.runtimeReady) {
        ImGui::Spacing();
        ImGui::TextWrapped("%s", nightclub.detail.c_str());
        return;
    }

    DrawStatusValue("Nightclub", nightclub.owned ? "OWNED" : "NOT OWNED");

    ImGui::TextDisabled("Property / Warehouse Index");
    ImGui::SameLine(190.0F);
    ImGui::Text("%d / %d", nightclub.nightclubIndex, nightclub.businessHubIndex);

    ImGui::TextDisabled("Popularity");
    ImGui::SameLine(190.0F);
    ImGui::Text("%.0f%%", nightclub.popularity * 100.0F);

    DrawCurrencyValue("Safe Cash", nightclub.safeCash);
    DrawCurrencyValue("Entry Cost", nightclub.entryCost);

    int totalUnits = 0;
    int totalCapacity = 0;
    long long estimatedValue = 0;
    int availableGoods = 0;

    for (const auto& good : nightclub.goods) {
        if (!good.available)
            continue;

        ++availableGoods;
        totalUnits += good.units;
        totalCapacity += good.maxUnits;
        estimatedValue += static_cast<long long>(good.units) * good.unitValue;
    }

    ImGui::TextDisabled("Warehouse Goods");
    ImGui::SameLine(190.0F);
    ImGui::Text("%d / %d", totalUnits, totalCapacity);

    ImGui::TextDisabled("Estimated Value");
    ImGui::SameLine(190.0F);
    ImGui::Text("$%lld", estimatedValue);

    ImGui::TextDisabled("Goods Available");
    ImGui::SameLine(190.0F);
    ImGui::Text("%d / %zu", availableGoods, GTA_Nightclub_Good_Count);

    DrawDivider();
    ImGui::TextDisabled("Runtime Detail");
    ImGui::TextWrapped("%s", nightclub.detail.c_str());
}

void DrawNightclubGoods(const GTA_Nightclub_Snapshot& nightclub)
{
    auto& theme = Themes::Menu_Theme_Manager::Instance();
    const auto& palette = theme.Palette();

    ImGui::TextColored(palette.bronze, "NIGHTCLUB WAREHOUSE");
    DrawDivider();

    if (!nightclub.runtimeReady) {
        ImGui::TextColored(
            ImVec4{0.90F, 0.58F, 0.18F, 1.0F},
            "Business runtime unavailable");
        ImGui::TextWrapped("%s", nightclub.detail.c_str());
        return;
    }

    if (!nightclub.owned) {
        ImGui::TextDisabled("No owned Nightclub is currently reported for the local player.");
        ImGui::Spacing();
    }

    ImGui::TextDisabled("Popularity");
    const float popularity = nightclub.popularity < 0.0F
        ? 0.0F
        : (nightclub.popularity > 1.0F ? 1.0F : nightclub.popularity);
    ImGui::ProgressBar(popularity, ImVec2{-1.0F, 0.0F});

    DrawCurrencyValue("Safe Cash", nightclub.safeCash);

    ImGui::TextDisabled("Equipment Multiplier");
    ImGui::SameLine(190.0F);
    ImGui::Text("%.2fx", nightclub.equipmentUpgradeMultiplier);

    DrawDivider();

    constexpr ImGuiTableFlags tableFlags =
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_SizingStretchProp;

    if (ImGui::BeginTable("##NightclubGoods", 6, tableFlags)) {
        ImGui::TableSetupColumn("GOOD");
        ImGui::TableSetupColumn("STOCK");
        ImGui::TableSetupColumn("UNIT VALUE");
        ImGui::TableSetupColumn("SPECIAL ORDER");
        ImGui::TableSetupColumn("EST. VALUE");
        ImGui::TableSetupColumn("PRODUCTION");
        ImGui::TableHeadersRow();

        for (std::size_t index = 0; index < GTA_Nightclub_Good_Count; ++index) {
            const auto goodId = static_cast<GTA_Nightclub_Good>(index);
            const auto& good = nightclub.goods[index];

            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(GTA_Nightclub_Good_Name(goodId));

            if (!good.available) {
                ImGui::TableSetColumnIndex(1);
                ImGui::TextDisabled("Unavailable");
                continue;
            }

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%d / %d", good.units, good.maxUnits);

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("$%d", good.unitValue);

            ImGui::TableSetColumnIndex(3);
            ImGui::Text("$%d", good.specialOrderUnitValue);

            ImGui::TableSetColumnIndex(4);
            const auto estimatedValue =
                static_cast<long long>(good.units) * good.unitValue;
            ImGui::Text("$%lld", estimatedValue);

            ImGui::TableSetColumnIndex(5);
            const float baseMinutes =
                static_cast<float>(good.productionTimeMs) / 60000.0F;
            const float upgradedMinutes =
                baseMinutes * nightclub.equipmentUpgradeMultiplier;

            if (nightclub.equipmentUpgradeMultiplier > 0.0F &&
                nightclub.equipmentUpgradeMultiplier < 1.0F) {
                ImGui::Text("%.0f / %.0f min", baseMinutes, upgradedMinutes);
            } else {
                ImGui::Text("%.0f min", baseMinutes);
            }
        }

        ImGui::EndTable();
    }
}

void DrawNightclubActionStatus(const GTA_Nightclub_Action_Snapshot& action)
{
    if (action.status == GTA_Nightclub_Action_Status::Idle)
        return;

    ImVec4 color{0.90F, 0.58F, 0.18F, 1.0F};
    if (action.status == GTA_Nightclub_Action_Status::Succeeded)
        color = ImVec4{0.42F, 0.78F, 0.42F, 1.0F};
    else if (action.status != GTA_Nightclub_Action_Status::Queued)
        color = ImVec4{0.92F, 0.32F, 0.28F, 1.0F};

    ImGui::TextColored(
        color,
        "[%s] %s",
        GTA_Nightclub_Action_Status_Name(action.status),
        GTA_Nightclub_Action_Name(action.kind));
    if (!action.detail.empty())
        ImGui::TextWrapped("%s", action.detail.c_str());
    ImGui::Spacing();
}

void DrawResupplyActionStatus(const GTA_Resupply_Action_Snapshot& action)
{
    if (action.status == GTA_Resupply_Action_Status::Idle)
        return;

    ImVec4 color{0.90F, 0.58F, 0.18F, 1.0F};
    if (action.status == GTA_Resupply_Action_Status::Succeeded)
        color = ImVec4{0.42F, 0.78F, 0.42F, 1.0F};
    else if (action.status != GTA_Resupply_Action_Status::Queued)
        color = ImVec4{0.92F, 0.32F, 0.28F, 1.0F};

    ImGui::TextColored(
        color,
        "[%s] %s",
        GTA_Resupply_Action_Status_Name(action.status),
        GTA_Resupply_Action_Name(action.kind));

    if (action.kind == GTA_Resupply_Action_Kind::ResupplyTarget) {
        ImGui::SameLine();
        ImGui::TextDisabled("- %s", GTA_Resupply_Target_Name(action.target));
    }

    if (!action.detail.empty())
        ImGui::TextWrapped("%s", action.detail.c_str());
    ImGui::Spacing();
}

void DrawNightclubCoreEditor(NightclubEditorState& editor)
{
    auto& business = GTA_Business_State::Instance();

    ImGui::TextDisabled("Popularity");
    ImGui::SetNextItemWidth(320.0F);
    ImGui::SliderFloat(
        "##NightclubPopularityTarget",
        &editor.popularityPercent,
        0.0F,
        100.0F,
        "%.0f%%");

    ImGui::SetNextItemWidth(220.0F);
    ImGui::InputInt("Safe Cash", &editor.safeCash, 10'000, 50'000);
    ImGui::SetNextItemWidth(220.0F);
    ImGui::InputInt("Entry Cost", &editor.entryCost, 10, 100);

    if (ImGui::Button("APPLY CORE VALUES", ImVec2{180.0F, 34.0F})) {
        editor.popularityPercent = std::clamp(editor.popularityPercent, 0.0F, 100.0F);
        editor.safeCash = std::max(editor.safeCash, 0);
        editor.entryCost = std::max(editor.entryCost, 0);
        (void)business.RequestNightclubCoreValues(
            editor.popularityPercent / 100.0F,
            editor.safeCash,
            editor.entryCost);
    }

    ImGui::SameLine();
    if (ImGui::Button("MAX POPULARITY", ImVec2{160.0F, 34.0F})) {
        editor.popularityPercent = 100.0F;
        (void)business.RequestNightclubPopularity(1.0F);
    }

    ImGui::SameLine();
    if (ImGui::Button("FILL SAFE", ImVec2{120.0F, 34.0F})) {
        editor.safeCash = GTA_Nightclub_Safe_Capacity;
        (void)business.RequestNightclubSafeCash(GTA_Nightclub_Safe_Capacity);
    }
}

void DrawNightclubWarehouseEditor(
    const GTA_Nightclub_Snapshot& nightclub,
    NightclubEditorState& editor)
{
    auto& business = GTA_Business_State::Instance();

    DrawDivider();
    ImGui::TextDisabled("WAREHOUSE STOCK TARGETS");
    ImGui::Spacing();

    if (ImGui::BeginTable(
            "##NightclubStockEditor",
            3,
            ImGuiTableFlags_Borders |
                ImGuiTableFlags_RowBg |
                ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("GOOD");
        ImGui::TableSetupColumn("LIVE");
        ImGui::TableSetupColumn("TARGET");
        ImGui::TableHeadersRow();

        for (std::size_t index = 0; index < GTA_Nightclub_Good_Count; ++index) {
            const auto goodId = static_cast<GTA_Nightclub_Good>(index);
            const auto& good = nightclub.goods[index];

            ImGui::PushID(static_cast<int>(index));
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(GTA_Nightclub_Good_Name(goodId));
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%d / %d", good.units, good.maxUnits);
            ImGui::TableSetColumnIndex(2);
            ImGui::SetNextItemWidth(-1.0F);
            ImGui::InputInt("##StockTarget", &editor.productStocks[index]);
            ImGui::PopID();
        }

        ImGui::EndTable();
    }

    if (ImGui::Button("APPLY WAREHOUSE", ImVec2{180.0F, 34.0F})) {
        for (std::size_t index = 0; index < GTA_Nightclub_Good_Count; ++index) {
            const int maxUnits = std::max(nightclub.goods[index].maxUnits, 0);
            editor.productStocks[index] = std::clamp(
                editor.productStocks[index],
                0,
                maxUnits);
        }
        (void)business.RequestNightclubProductStocks(editor.productStocks);
    }

    ImGui::SameLine();
    if (ImGui::Button("FILL WAREHOUSE", ImVec2{170.0F, 34.0F})) {
        for (std::size_t index = 0; index < GTA_Nightclub_Good_Count; ++index)
            editor.productStocks[index] = std::max(nightclub.goods[index].maxUnits, 0);
        (void)business.RequestFillNightclubProductStocks();
    }

    ImGui::SameLine();
    if (ImGui::Button("CLEAR WAREHOUSE", ImVec2{170.0F, 34.0F})) {
        editor.productStocks.fill(0);
        (void)business.RequestClearNightclubProductStocks();
    }
}

void DrawNightclubSaleEditor(
    const GTA_Nightclub_Snapshot& nightclub,
    NightclubEditorState& editor)
{
    DrawDivider();
    if (!ImGui::CollapsingHeader("LIVE SALE VALUES (ADVANCED)"))
        return;

    ImGui::TextDisabled(
        "These GPBD_FM values can be recalculated by the active Nightclub sale script.");
    ImGui::Text(
        "Buyer: %d / %d | Partial amount: $%d",
        nightclub.sale.buyerIndex,
        nightclub.sale.buyerIndex2,
        nightclub.sale.unknownPartialAmount);
    ImGui::Spacing();

    ImGui::SetNextItemWidth(260.0F);
    ImGui::InputInt("Sold Items", &editor.sale.soldItems);
    ImGui::SetNextItemWidth(260.0F);
    ImGui::InputInt("Base Sale Amount", &editor.sale.saleAmount, 10'000, 100'000);
    ImGui::SetNextItemWidth(260.0F);
    ImGui::InputInt(
        "Sale Amount With Membership Modifiers",
        &editor.sale.saleAmountWithMembershipModifiers,
        10'000,
        100'000);
    ImGui::SetNextItemWidth(260.0F);
    ImGui::InputInt(
        "Total With Membership Modifiers",
        &editor.sale.totalSaleAmountWithMembershipModifiers,
        10'000,
        100'000);
    ImGui::SetNextItemWidth(260.0F);
    ImGui::InputInt(
        "Total Sale Amount",
        &editor.sale.totalSaleAmount,
        10'000,
        100'000);

    if (ImGui::Button("APPLY LIVE SALE VALUES", ImVec2{230.0F, 34.0F})) {
        editor.sale.soldItems = std::max(editor.sale.soldItems, 0);
        editor.sale.saleAmount = std::max(editor.sale.saleAmount, 0);
        editor.sale.saleAmountWithMembershipModifiers =
            std::max(editor.sale.saleAmountWithMembershipModifiers, 0);
        editor.sale.totalSaleAmountWithMembershipModifiers =
            std::max(editor.sale.totalSaleAmountWithMembershipModifiers, 0);
        editor.sale.totalSaleAmount = std::max(editor.sale.totalSaleAmount, 0);
        (void)GTA_Business_State::Instance().RequestNightclubSaleValues(editor.sale);
    }
}

void DrawNightclubMissionEditor(
    const GTA_Nightclub_Snapshot& nightclub,
    NightclubEditorState& editor)
{
    ImGui::Spacing();
    if (!ImGui::CollapsingHeader("MISSION STATE (ADVANCED)"))
        return;

    ImGui::TextDisabled(
        "These values use the registered GPBD_FM_3 local-player entry. Use -1 to clear.");
    ImGui::Text(
        "Live indexes: mission %d | defense %d",
        nightclub.missionIndex,
        nightclub.defendMissionIndex);
    ImGui::Spacing();

    ImGui::SetNextItemWidth(220.0F);
    ImGui::InputInt("Mission Index", &editor.missionIndex);
    ImGui::SetNextItemWidth(220.0F);
    ImGui::InputInt("Defense Mission Index", &editor.defendMissionIndex);

    if (ImGui::Button("APPLY MISSION INDEXES", ImVec2{220.0F, 34.0F})) {
        editor.missionIndex = std::max(editor.missionIndex, -1);
        editor.defendMissionIndex = std::max(editor.defendMissionIndex, -1);
        (void)GTA_Business_State::Instance().RequestNightclubMissionState(
            editor.missionIndex,
            editor.defendMissionIndex);
    }

    ImGui::SameLine();
    if (ImGui::Button("RESET MISSION STATE", ImVec2{210.0F, 34.0F})) {
        editor.missionIndex = -1;
        editor.defendMissionIndex = -1;
        (void)GTA_Business_State::Instance().RequestResetNightclubMissionState();
    }
}

void DrawNightclubControls(
    const GTA_Nightclub_Snapshot& nightclub,
    const GTA_Nightclub_Action_Snapshot& action,
    NightclubEditorState& editor)
{
    auto& theme = Themes::Menu_Theme_Manager::Instance();
    const auto& palette = theme.Palette();

    DrawDivider();
    ImGui::TextColored(palette.bronze, "NIGHTCLUB CONTROLS");
    ImGui::TextDisabled(
        "All writes are queued onto the GTA game thread and verified by immediate readback.");
    ImGui::Spacing();
    DrawNightclubActionStatus(action);

    if (nightclub.runtimeReady && nightclub.owned &&
        ImGui::Button("RELOAD LIVE VALUES", ImVec2{190.0F, 30.0F})) {
        editor.Load(nightclub);
    }

    const bool actionQueued = action.status == GTA_Nightclub_Action_Status::Queued;
    const bool disabled = !nightclub.runtimeReady ||
        !nightclub.owned ||
        !editor.initialized ||
        actionQueued;

    if (actionQueued) {
        ImGui::SameLine();
        ImGui::TextDisabled("Waiting for the game thread...");
    }

    ImGui::BeginDisabled(disabled);
    DrawNightclubCoreEditor(editor);
    DrawNightclubWarehouseEditor(nightclub, editor);
    DrawNightclubSaleEditor(nightclub, editor);
    DrawNightclubMissionEditor(nightclub, editor);
    ImGui::EndDisabled();
}

void DrawNightclubTunables(const GTA_Nightclub_Snapshot& nightclub)
{
    auto& theme = Themes::Menu_Theme_Manager::Instance();
    const auto& palette = theme.Palette();

    ImGui::TextColored(palette.bronze, "NIGHTCLUB TUNABLE STATUS");
    DrawDivider();

    if (!nightclub.runtimeReady) {
        ImGui::TextDisabled("Nightclub tunables are unavailable until the business runtime is ready.");
        return;
    }

    ImGui::TextDisabled("Management Mission Cooldown");
    ImGui::SameLine(250.0F);
    ImGui::Text("%.1f min", static_cast<float>(nightclub.managementMissionCooldownMs) / 60000.0F);

    ImGui::TextDisabled("Sell Mission Cooldown");
    ImGui::SameLine(250.0F);
    ImGui::Text("%.1f min", static_cast<float>(nightclub.sellMissionCooldownMs) / 60000.0F);

    ImGui::TextDisabled("Special Order Sell Cooldown");
    ImGui::SameLine(250.0F);
    ImGui::Text("%.1f min", static_cast<float>(nightclub.specialOrderSellCooldownMs) / 60000.0F);

    DrawDivider();
    ImGui::TextColored(palette.bronze, "POPULARITY INCOME TIERS");
    ImGui::Spacing();

    if (ImGui::BeginTable(
            "##NightclubPopularityIncome",
            2,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("POPULARITY");
        ImGui::TableSetupColumn("INCOME");
        ImGui::TableHeadersRow();

        for (std::size_t index = 0; index < nightclub.popularityIncome.size(); ++index) {
            const int low = static_cast<int>(index * 5U) + 1;
            const int high = low + 4;

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%d - %d", low, high);
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("$%d", nightclub.popularityIncome[index]);
        }

        ImGui::EndTable();
    }
}

void DrawInstantResupplyControls(
    const GTA_Nightclub_Snapshot& runtime,
    const GTA_Resupply_Action_Snapshot& action)
{
    auto& theme = Themes::Menu_Theme_Manager::Instance();
    const auto& palette = theme.Palette();
    auto& business = GTA_Business_State::Instance();

    ImGui::TextColored(palette.bronze, "INSTANT RESUPPLY");
    DrawDivider();
    ImGui::TextWrapped(
        "Each action writes trigger value 1 to the Enhanced 1.73 freemode business "
        "command block (Global_1673820 + 1 through + 7). The game consumes the trigger.");
    ImGui::TextDisabled(
        "Slots 0-4 are MC factory slots and can contain Cash, Coke, Weed, Meth, or Documents.");
    ImGui::TextDisabled("Slot 5 is the Bunker; slot 6 is the Acid Lab.");
    ImGui::Spacing();

    DrawResupplyActionStatus(action);

    if (!runtime.runtimeReady) {
        ImGui::TextColored(
            ImVec4{0.90F, 0.58F, 0.18F, 1.0F},
            "Business runtime unavailable");
        ImGui::TextWrapped("%s", runtime.detail.c_str());
        return;
    }

    const bool actionQueued = action.status == GTA_Resupply_Action_Status::Queued;
    if (actionQueued)
        ImGui::TextDisabled("Waiting for the GTA game thread...");

    ImGui::BeginDisabled(actionQueued);

    constexpr ImGuiTableFlags tableFlags =
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_SizingStretchProp;

    if (ImGui::BeginTable("##InstantResupplyTargets", 3, tableFlags)) {
        ImGui::TableSetupColumn("TARGET");
        ImGui::TableSetupColumn("GLOBAL");
        ImGui::TableSetupColumn("ACTION");
        ImGui::TableHeadersRow();

        for (std::size_t index = 0; index < GTA_Resupply_Target_Count; ++index) {
            const auto target = static_cast<GTA_Resupply_Target>(index);
            ImGui::PushID(static_cast<int>(index));
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(GTA_Resupply_Target_Name(target));
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("Global_1673820 + %zu", index + 1U);
            ImGui::TableSetColumnIndex(2);
            if (ImGui::Button("RESUPPLY", ImVec2{130.0F, 0.0F}))
                (void)business.RequestInstantResupply(target);
            ImGui::PopID();
        }

        ImGui::EndTable();
    }

    ImGui::Spacing();
    if (ImGui::Button("RESUPPLY ALL", ImVec2{180.0F, 34.0F}))
        (void)business.RequestInstantResupplyAll();

    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::TextDisabled("Only owned, set-up businesses can consume a resupply trigger.");
}
}

void DrawMiscPage()
{
    auto& theme = Themes::Menu_Theme_Manager::Instance();
    const auto& palette = theme.Palette();
    auto& business = GTA_Business_State::Instance();
    const auto nightclub = business.Nightclub();
    const auto action = business.NightclubAction();
    const auto resupplyAction = business.ResupplyAction();
    auto& editor = EditorState();

    if (nightclub.runtimeReady && nightclub.owned &&
        (!editor.initialized ||
         editor.nightclubIndex != nightclub.nightclubIndex ||
         editor.businessHubIndex != nightclub.businessHubIndex)) {
        editor.Load(nightclub);
    } else if (!nightclub.runtimeReady || !nightclub.owned) {
        editor.initialized = false;
    }

    ImGui::TextColored(palette.emberRed, "MISC");
    ImGui::SameLine();
    ImGui::TextDisabled("- GTA Online businesses");

    DrawDivider();

    ImGui::TextColored(palette.bronze, "BUSINESS RUNTIME");
    ImGui::SameLine();
    if (nightclub.runtimeReady) {
        ImGui::TextColored(ImVec4{0.42F, 0.78F, 0.42F, 1.0F}, "[CONNECTED]");
    } else {
        ImGui::TextColored(ImVec4{0.90F, 0.58F, 0.18F, 1.0F}, "[WAITING]");
    }

    ImGui::TextWrapped(
        "Business data and queued write actions are provided by the GTA runtime backend. "
        "The registered globals target Enhanced 1.73 / b1158.13 only.");
    ImGui::Spacing();

    if (ImGui::BeginTabBar("##MiscPageTabs")) {
        if (ImGui::BeginTabItem("OVERVIEW")) {
            DrawOverview(nightclub);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("NIGHTCLUB")) {
            DrawNightclubGoods(nightclub);
            DrawNightclubControls(nightclub, action, editor);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("RESUPPLY")) {
            DrawInstantResupplyControls(nightclub, resupplyAction);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("TUNABLES")) {
            DrawNightclubTunables(nightclub);
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}
}
