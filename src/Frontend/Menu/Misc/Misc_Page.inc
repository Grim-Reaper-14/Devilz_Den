#include "Misc_Page.hpp"

#include "Frontend/Menu/Themes/Menu_Theme.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Business_State.hpp"

#include <imgui.h>

#include <cstddef>

namespace Devilz::Frontend::Misc
{
namespace
{
using Integrations::GTA5_Enhanced::GTA_Business_State;
using Integrations::GTA5_Enhanced::GTA_Nightclub_Good;
using Integrations::GTA5_Enhanced::GTA_Nightclub_Good_Count;
using Integrations::GTA5_Enhanced::GTA_Nightclub_Good_Name;
using Integrations::GTA5_Enhanced::GTA_Nightclub_Snapshot;

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

    ImGui::TextDisabled("Popularity");
    ImGui::SameLine(190.0F);
    ImGui::Text("%.0f%%", nightclub.popularity * 100.0F);

    DrawCurrencyValue("Safe Cash", nightclub.safeCash);

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
}

void DrawMiscPage()
{
    auto& theme = Themes::Menu_Theme_Manager::Instance();
    const auto& palette = theme.Palette();
    const auto nightclub = GTA_Business_State::Instance().Nightclub();

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
        "Business data is read from the GTA runtime backend. This page currently exposes "
        "the read-only Nightclub integration for Enhanced 1.73.");
    ImGui::Spacing();

    if (ImGui::BeginTabBar("##MiscPageTabs")) {
        if (ImGui::BeginTabItem("OVERVIEW")) {
            DrawOverview(nightclub);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("NIGHTCLUB")) {
            DrawNightclubGoods(nightclub);
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
