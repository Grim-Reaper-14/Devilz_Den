#include "Frontend/Menu/Themes/Menu_Theme.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Clothing_Unlocks.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Packed_Stats_State.hpp"

#include <imgui.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace Devilz::Frontend::Recovery
{
namespace
{
using Integrations::GTA5_Enhanced::GTAAllClothingUnlockIndices;
using Integrations::GTA5_Enhanced::GTAClothingUnlockIndices;
using Integrations::GTA5_Enhanced::GTA_Clothing_Unlock_Groups;
using Integrations::GTA5_Enhanced::GTA_Clothing_Unlock_Source_Version;
using Integrations::GTA5_Enhanced::GTA_Clothing_Unlock_Unique_Count;
using Integrations::GTA5_Enhanced::GTA_Packed_Stats_State;

struct ClothingUnlockUiState
{
    std::uint64_t batchId = 0;
    std::string label;
};

ClothingUnlockUiState& ClothingState()
{
    static ClothingUnlockUiState state;
    return state;
}

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

void QueueClothingUnlocks(
    std::string_view label,
    std::vector<std::int32_t> indices)
{
    if (indices.empty())
        return;

    auto& state = ClothingState();
    state.label.assign(label);
    state.batchId = GTA_Packed_Stats_State::Instance().RequestWrites(indices, true);
}

void DrawClothingBatchStatus()
{
    const auto batch = GTA_Packed_Stats_State::Instance().BatchSnapshot();
    const auto& state = ClothingState();
    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();

    if (state.batchId == 0) {
        ImGui::TextDisabled("No clothing unlock batch has been queued this session.");
        return;
    }

    if (batch.batchId != state.batchId) {
        ImGui::TextDisabled(
            "The %s batch was replaced by another packed-stat operation.",
            state.label.c_str());
        return;
    }

    const float progress = batch.total == 0
        ? 0.0F
        : static_cast<float>(batch.completed) / static_cast<float>(batch.total);
    ImGui::TextColored(palette.bronze, "%s", state.label.c_str());
    ImGui::ProgressBar(progress, ImVec2{-1.0F, 0.0F});
    ImGui::TextDisabled(
        "%zu / %zu complete | %zu failed",
        batch.completed,
        batch.total,
        batch.failed);

    if (!batch.active && batch.completed == batch.total) {
        if (batch.failed == 0) {
            ImGui::TextColored(
                ImVec4{0.42F, 0.78F, 0.42F, 1.0F},
                "[SUCCEEDED] Clothing packed BOOLs were applied.");
        } else {
            ImGui::TextColored(
                ImVec4{0.92F, 0.32F, 0.28F, 1.0F},
                "[PARTIAL] %zu packed BOOL writes failed.",
                batch.failed);
        }
    }
}

void DrawClothingUnlockTable(bool batchActive)
{
    constexpr auto tableFlags =
        ImGuiTableFlags_BordersInnerH |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_SizingStretchProp;

    if (!ImGui::BeginTable(
            "##ClothingUnlockDlcTable",
            3,
            tableFlags,
            ImVec2{0.0F, 390.0F})) {
        return;
    }

    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("DLC", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("BOOLS", ImGuiTableColumnFlags_WidthFixed, 72.0F);
    ImGui::TableSetupColumn("ACTION", ImGuiTableColumnFlags_WidthFixed, 108.0F);
    ImGui::TableHeadersRow();

    for (const auto& group : GTA_Clothing_Unlock_Groups) {
        ImGui::PushID(group.internalTag.data());
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(
            group.name.data(),
            group.name.data() + group.name.size());
        ImGui::TextDisabled(
            "%.*s",
            static_cast<int>(group.internalTag.size()),
            group.internalTag.data());

        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%zu", group.expectedCount);

        ImGui::TableSetColumnIndex(2);
        ImGui::BeginDisabled(batchActive);
        if (ImGui::Button("UNLOCK", ImVec2{92.0F, 28.0F})) {
            QueueClothingUnlocks(
                group.name,
                GTAClothingUnlockIndices(group));
        }
        ImGui::EndDisabled();
        ImGui::PopID();
    }

    ImGui::EndTable();
}

void DrawClothingUnlocks()
{
    const auto batch = GTA_Packed_Stats_State::Instance().BatchSnapshot();
    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();

    ImGui::TextColored(palette.bronze, "CLOTHING UNLOCKS");
    ImGui::TextWrapped(
        "Curated packed BOOL presets verified against GTA Online %.*s. "
        "Use one DLC or queue all %zu unique apparel flags.",
        static_cast<int>(GTA_Clothing_Unlock_Source_Version.size()),
        GTA_Clothing_Unlock_Source_Version.data(),
        GTA_Clothing_Unlock_Unique_Count);
    ImGui::Spacing();

    ImGui::BeginDisabled(batch.active);
    if (ImGui::Button("UNLOCK ALL CLOTHING", ImVec2{210.0F, 36.0F}))
        ImGui::OpenPopup("Confirm Clothing Unlock All");
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::TextDisabled("Persistent packed-stat changes");

    if (ImGui::BeginPopupModal(
            "Confirm Clothing Unlock All",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped(
            "This will set %zu clothing packed BOOLs to true for the active online character. "
            "The operation is persistent and cannot be automatically rolled back.",
            GTA_Clothing_Unlock_Unique_Count);
        ImGui::Spacing();
        ImGui::BeginDisabled(batch.active);
        if (ImGui::Button("CONFIRM UNLOCK", ImVec2{170.0F, 34.0F})) {
            QueueClothingUnlocks(
                "Unlock All Clothing",
                GTAAllClothingUnlockIndices());
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("CANCEL", ImVec2{110.0F, 34.0F}))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    DrawDivider();
    DrawClothingBatchStatus();
    DrawDivider();
    DrawClothingUnlockTable(batch.active);

    ImGui::Spacing();
    ImGui::TextDisabled(
        "Sparse reward lists are preserved; unrelated Car Meet and Arena packed stats are not written.");
    ImGui::TextDisabled(
        "Seasonal Valentine and Halloween storefront availability still depends on live tunables.");
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
    ImGui::TextColored(ImVec4{0.42F, 0.78F, 0.42F, 1.0F}, "[PACKED PRESETS READY]");

    ImGui::TextWrapped(
        "Recovery owns curated progression and unlock presets. Writes are queued onto the GTA "
        "game thread through the Enhanced packed-stat runtime.");
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
            DrawClothingUnlocks();
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
            DrawClothingBatchStatus();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}
}
