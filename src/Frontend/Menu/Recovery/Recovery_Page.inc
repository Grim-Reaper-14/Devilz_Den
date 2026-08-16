#include "Frontend/Menu/Themes/Menu_Theme.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Career_Progress_Unlocks.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Clothing_Unlocks.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Collection_Unlocks.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Halloween_Unlocks.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Packed_Stats_State.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Stats_Extension.hpp"

#include <imgui.h>

#include <array>
#include <charconv>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace Devilz::Frontend::Recovery
{
namespace
{
using Integrations::GTA5_Enhanced::GTAAllClothingUnlockIndices;
using Integrations::GTA5_Enhanced::GTAAllCollectionUnlockIndices;
using Integrations::GTA5_Enhanced::GTAAllVerifiedHalloweenUnlockIndices;
using Integrations::GTA5_Enhanced::GTACareerProgressCompletedTierCount;
using Integrations::GTA5_Enhanced::GTACareerProgressIsComplete;
using Integrations::GTA5_Enhanced::GTACareerProgressMaskValue;
using Integrations::GTA5_Enhanced::GTACareerProgressRequestName;
using Integrations::GTA5_Enhanced::GTAClothingUnlockIndices;
using Integrations::GTA5_Enhanced::GTACollectionUnlockIndices;
using Integrations::GTA5_Enhanced::GTAHalloweenGhostsExposed2023Indices;
using Integrations::GTA5_Enhanced::GTAHalloweenGhostsExposed2024Indices;
using Integrations::GTA5_Enhanced::GTAHalloweenGhostsExposed2025Indices;
using Integrations::GTA5_Enhanced::GTAHalloweenJackOLanternIndices;
using Integrations::GTA5_Enhanced::GTA_Career_Progress_All_Stat_Masks;
using Integrations::GTA5_Enhanced::GTA_Career_Progress_Completion_Bit_Count;
using Integrations::GTA5_Enhanced::GTA_Career_Progress_Count;
using Integrations::GTA5_Enhanced::GTA_Career_Progress_Entries;
using Integrations::GTA5_Enhanced::GTA_Career_Progress_Entry;
using Integrations::GTA5_Enhanced::GTA_Career_Progress_Source;
using Integrations::GTA5_Enhanced::GTA_Career_Progress_Tiers_Per_Career;
using Integrations::GTA5_Enhanced::GTA_Clothing_Unlock_Groups;
using Integrations::GTA5_Enhanced::GTA_Clothing_Unlock_Source_Version;
using Integrations::GTA5_Enhanced::GTA_Clothing_Unlock_Unique_Count;
using Integrations::GTA5_Enhanced::GTA_Collection_Unlock_Groups;
using Integrations::GTA5_Enhanced::GTA_Collection_Unlock_Source;
using Integrations::GTA5_Enhanced::GTA_Collection_Unlock_Unique_Count;
using Integrations::GTA5_Enhanced::GTA_Halloween_Ghosts_Exposed_2023_Count;
using Integrations::GTA5_Enhanced::GTA_Halloween_Ghosts_Exposed_2024_Count;
using Integrations::GTA5_Enhanced::GTA_Halloween_Ghosts_Exposed_2025_Count;
using Integrations::GTA5_Enhanced::GTA_Halloween_Jack_O_Lantern_Count;
using Integrations::GTA5_Enhanced::GTA_Halloween_Unlock_Source;
using Integrations::GTA5_Enhanced::GTA_Halloween_Verified_Unique_Count;
using Integrations::GTA5_Enhanced::GTA_Packed_Stats_State;
using Integrations::GTA5_Enhanced::GTA_Stat_Request_Status;
using Integrations::GTA5_Enhanced::GTA_Stat_Snapshot;
using Integrations::GTA5_Enhanced::GTA_Stats_State;
using Integrations::GTA5_Enhanced::GTAStatRequestStatusName;

struct UnlockBatchUiState
{
    std::uint64_t batchId = 0;
    std::string label;
};

struct CharacterUnlockUiState
{
    std::uint64_t requestId = 0;
};

struct CareerProgressUiState
{
    std::vector<std::uint64_t> requestIds;
    std::string label;
    std::array<std::uint64_t, GTA_Career_Progress_All_Stat_Masks.size()> refreshRequestIds{};
    std::array<std::uint32_t, GTA_Career_Progress_All_Stat_Masks.size()> fieldValues{};
    bool snapshotValid = false;
};

UnlockBatchUiState& UnlockState()
{
    static UnlockBatchUiState state;
    return state;
}

CharacterUnlockUiState& CharacterUnlockState()
{
    static CharacterUnlockUiState state;
    return state;
}

CareerProgressUiState& CareerState()
{
    static CareerProgressUiState state;
    return state;
}

void DrawDivider()
{
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}

[[nodiscard]] bool StatRequestFinished(GTA_Stat_Request_Status status) noexcept
{
    return status != GTA_Stat_Request_Status::Idle &&
        status != GTA_Stat_Request_Status::Queued;
}

[[nodiscard]] bool ParseSignedStatBits(std::string_view text, std::uint32_t& bits) noexcept
{
    std::int32_t value = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size())
        return false;
    bits = static_cast<std::uint32_t>(value);
    return true;
}

[[nodiscard]] std::size_t CareerFieldSlot(std::int32_t statIndex) noexcept
{
    for (std::size_t i = 0; i < GTA_Career_Progress_All_Stat_Masks.size(); ++i) {
        if (GTA_Career_Progress_All_Stat_Masks[i].statIndex == statIndex)
            return i;
    }
    return GTA_Career_Progress_All_Stat_Masks.size();
}

void QueuePackedBoolUnlocks(
    std::string_view label,
    std::vector<std::int32_t> indices)
{
    if (indices.empty())
        return;

    auto& state = UnlockState();
    state.label.assign(label);
    state.batchId = GTA_Packed_Stats_State::Instance().RequestWrites(indices, true);
}

void DrawUnlockBatchStatus()
{
    const auto batch = GTA_Packed_Stats_State::Instance().BatchSnapshot();
    const auto& state = UnlockState();
    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();

    if (state.batchId == 0) {
        ImGui::TextDisabled("No unlock batch has been queued this session.");
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
                "[SUCCEEDED] Packed BOOLs were applied.");
        } else {
            ImGui::TextColored(
                ImVec4{0.92F, 0.32F, 0.28F, 1.0F},
                "[PARTIAL] %zu packed BOOL writes failed.",
                batch.failed);
        }
    }
}

void DrawCharacterUnlocks()
{
    auto& stats = GTA_Stats_State::Instance();
    auto& state = CharacterUnlockState();
    const auto metrics = stats.QueueSnapshot();
    GTA_Stat_Snapshot result{};
    if (state.requestId != 0) {
        const auto snapshot = stats.Snapshot(state.requestId);
        if (snapshot)
            result = *snapshot;
    }
    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();
    const bool busy = state.requestId != 0 &&
        result.requestId == state.requestId &&
        !StatRequestFinished(result.status);
    const bool queueFull = metrics.pending + metrics.inFlight >= metrics.capacity;

    ImGui::TextColored(palette.bronze, "CHARACTER UNLOCKS");
    ImGui::TextWrapped(
        "Allow Gender Change writes MPX_ALLOW_GENDER_CHANGE through the verified regular-stat queue. "
        "The value 52 matches YimMenuV2 Enhanced; Reset writes the default locked value 0.");
    ImGui::Spacing();

    ImGui::BeginDisabled(busy || queueFull);
    if (ImGui::Button("ALLOW GENDER CHANGE", ImVec2{210.0F, 36.0F}))
        state.requestId = stats.RequestWrite("MPX_ALLOW_GENDER_CHANGE", "52");
    ImGui::SameLine();
    if (ImGui::Button("RESET GENDER CHANGE", ImVec2{205.0F, 36.0F}))
        state.requestId = stats.RequestWrite("MPX_ALLOW_GENDER_CHANGE", "0");
    ImGui::EndDisabled();

    ImGui::TextDisabled(
        "Regular stat queue: %zu pending + %zu active / %zu capacity | %llu completed | %llu failed",
        metrics.pending,
        metrics.inFlight,
        metrics.capacity,
        static_cast<unsigned long long>(metrics.completed),
        static_cast<unsigned long long>(metrics.failed));

    if (state.requestId != 0 && result.requestId == state.requestId) {
        ImVec4 color = palette.bronze;
        if (result.status == GTA_Stat_Request_Status::Succeeded)
            color = ImVec4{0.42F, 0.78F, 0.42F, 1.0F};
        else if (StatRequestFinished(result.status))
            color = ImVec4{0.92F, 0.32F, 0.28F, 1.0F};

        ImGui::TextColored(color, "[%s]", GTAStatRequestStatusName(result.status));
        if (!result.detail.empty()) {
            ImGui::SameLine();
            ImGui::TextWrapped("%s", result.detail.c_str());
        }
    }
}

struct CareerRequestProgress
{
    std::size_t total = 0;
    std::size_t completed = 0;
    std::size_t failed = 0;
    bool active = false;
};

struct CareerCompletionSummary
{
    std::size_t completedCareers = 0;
    std::size_t completedTiers = 0;
};

[[nodiscard]] CareerRequestProgress CareerProgressStatus()
{
    CareerRequestProgress progress{};
    const auto& state = CareerState();
    auto& stats = GTA_Stats_State::Instance();
    progress.total = state.requestIds.size();

    for (const auto requestId : state.requestIds) {
        const auto snapshot = stats.Snapshot(requestId);
        if (!snapshot) {
            ++progress.completed;
            ++progress.failed;
            continue;
        }
        if (!StatRequestFinished(snapshot->status)) {
            progress.active = true;
            continue;
        }
        ++progress.completed;
        if (snapshot->status != GTA_Stat_Request_Status::Succeeded)
            ++progress.failed;
    }
    return progress;
}

[[nodiscard]] CareerRequestProgress CareerRefreshStatus()
{
    CareerRequestProgress progress{};
    auto& state = CareerState();
    auto& stats = GTA_Stats_State::Instance();
    bool anyRequest = false;
    bool allSucceeded = true;
    progress.total = state.refreshRequestIds.size();

    for (std::size_t i = 0; i < state.refreshRequestIds.size(); ++i) {
        const auto requestId = state.refreshRequestIds[i];
        if (requestId == 0) {
            allSucceeded = false;
            continue;
        }
        anyRequest = true;
        const auto snapshot = stats.Snapshot(requestId);
        if (!snapshot) {
            ++progress.completed;
            ++progress.failed;
            allSucceeded = false;
            continue;
        }
        if (!StatRequestFinished(snapshot->status)) {
            progress.active = true;
            allSucceeded = false;
            continue;
        }

        ++progress.completed;
        if (snapshot->status != GTA_Stat_Request_Status::Succeeded ||
            !ParseSignedStatBits(snapshot->value, state.fieldValues[i])) {
            ++progress.failed;
            allSucceeded = false;
        }
    }

    if (!anyRequest)
        progress.total = 0;
    if (anyRequest && !progress.active && progress.completed == state.refreshRequestIds.size())
        state.snapshotValid = allSucceeded && progress.failed == 0;
    return progress;
}

[[nodiscard]] CareerCompletionSummary CareerSummary() noexcept
{
    CareerCompletionSummary summary{};
    const auto& state = CareerState();
    if (!state.snapshotValid)
        return summary;

    for (const auto& entry : GTA_Career_Progress_Entries) {
        const auto slot = CareerFieldSlot(entry.statIndex);
        if (slot >= state.fieldValues.size())
            continue;
        const auto fieldValue = state.fieldValues[slot];
        summary.completedTiers += GTACareerProgressCompletedTierCount(entry, fieldValue);
        if (GTACareerProgressIsComplete(entry, fieldValue))
            ++summary.completedCareers;
    }
    return summary;
}

void QueueCareerProgressRefresh()
{
    auto& state = CareerState();
    auto& stats = GTA_Stats_State::Instance();
    state.snapshotValid = false;
    state.refreshRequestIds.fill(0);
    for (std::size_t i = 0; i < GTA_Career_Progress_All_Stat_Masks.size(); ++i) {
        state.refreshRequestIds[i] = stats.RequestRead(
            GTACareerProgressRequestName(GTA_Career_Progress_All_Stat_Masks[i].statIndex));
    }
}

void QueueCareerProgress(std::string_view label, const GTA_Career_Progress_Entry& entry)
{
    auto& state = CareerState();
    auto& stats = GTA_Stats_State::Instance();
    state.label.assign(label);
    state.requestIds.clear();
    state.snapshotValid = false;
    state.requestIds.push_back(stats.RequestWrite(
        GTACareerProgressRequestName(entry.statIndex),
        GTACareerProgressMaskValue(entry.completionMask)));
}

void QueueAllCareerProgress()
{
    auto& state = CareerState();
    auto& stats = GTA_Stats_State::Instance();
    state.label = "All Career Progress Tiers";
    state.requestIds.clear();
    state.snapshotValid = false;
    state.requestIds.reserve(GTA_Career_Progress_All_Stat_Masks.size());
    for (const auto& field : GTA_Career_Progress_All_Stat_Masks) {
        state.requestIds.push_back(stats.RequestWrite(
            GTACareerProgressRequestName(field.statIndex),
            GTACareerProgressMaskValue(field.completionMask)));
    }
}

void DrawCareerRefreshStatus()
{
    const auto refresh = CareerRefreshStatus();
    const auto& state = CareerState();
    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();
    if (refresh.total == 0) {
        ImGui::TextDisabled("Career tier status has not been refreshed this session.");
        return;
    }

    if (refresh.active) {
        const float ratio = static_cast<float>(refresh.completed) /
            static_cast<float>(refresh.total);
        ImGui::ProgressBar(ratio, ImVec2{-1.0F, 0.0F});
        ImGui::TextDisabled(
            "%zu / %zu Career stat fields read",
            refresh.completed,
            refresh.total);
        return;
    }

    if (state.snapshotValid) {
        const auto summary = CareerSummary();
        ImGui::TextColored(
            ImVec4{0.42F, 0.78F, 0.42F, 1.0F},
            "[CURRENT] %zu / %zu careers complete | %zu / %zu tiers complete",
            summary.completedCareers,
            GTA_Career_Progress_Count,
            summary.completedTiers,
            GTA_Career_Progress_Completion_Bit_Count);
    } else {
        ImGui::TextColored(
            ImVec4{0.92F, 0.32F, 0.28F, 1.0F},
            "[REFRESH FAILED] %zu of %zu Career stat reads failed or disappeared.",
            refresh.failed,
            refresh.total);
    }
}

void DrawCareerProgressStatus()
{
    const auto progress = CareerProgressStatus();
    const auto& state = CareerState();
    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();
    if (progress.total == 0) {
        ImGui::TextDisabled("No Career Progress write has been queued this session.");
        return;
    }

    const float ratio = progress.total == 0
        ? 0.0F
        : static_cast<float>(progress.completed) / static_cast<float>(progress.total);
    ImGui::TextColored(palette.bronze, "%s", state.label.c_str());
    ImGui::ProgressBar(ratio, ImVec2{-1.0F, 0.0F});
    ImGui::TextDisabled(
        "%zu / %zu stat fields complete | %zu failed",
        progress.completed,
        progress.total,
        progress.failed);

    if (!progress.active && progress.completed == progress.total) {
        if (progress.failed == 0) {
            ImGui::TextColored(
                ImVec4{0.42F, 0.78F, 0.42F, 1.0F},
                "[SUCCEEDED] Career completion bits were merged and verified. Refresh tier status to read current progress.");
        } else {
            ImGui::TextColored(
                ImVec4{0.92F, 0.32F, 0.28F, 1.0F},
                "[PARTIAL] %zu Career Progress stat writes failed.",
                progress.failed);
        }
    }
}

void DrawCareerProgression()
{
    auto& stats = GTA_Stats_State::Instance();
    const auto metrics = stats.QueueSnapshot();
    const auto progress = CareerProgressStatus();
    const auto refresh = CareerRefreshStatus();
    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();
    const std::size_t queueUsed = metrics.pending + metrics.inFlight;
    const std::size_t queueFree = metrics.capacity > queueUsed ? metrics.capacity - queueUsed : 0;
    const bool busy = progress.active || refresh.active;

    ImGui::TextColored(palette.bronze, "CAREER PROGRESS");
    ImGui::TextWrapped(
        "%zu Enhanced Career Progress tracks / %zu tier-completion bits traced from %.*s. "
        "Completion uses GTA's raw character-stat resolver and merges only the Career bits into the existing INT values.",
        GTA_Career_Progress_Count,
        GTA_Career_Progress_Completion_Bit_Count,
        static_cast<int>(GTA_Career_Progress_Source.size()),
        GTA_Career_Progress_Source.data());
    ImGui::Spacing();

    ImGui::BeginDisabled(busy || queueFree < GTA_Career_Progress_All_Stat_Masks.size());
    if (ImGui::Button("REFRESH TIER STATUS", ImVec2{190.0F, 36.0F}))
        QueueCareerProgressRefresh();
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::TextDisabled("read-only | 5 character-stat reads");
    DrawCareerRefreshStatus();
    ImGui::Spacing();

    ImGui::BeginDisabled(busy || queueFree < GTA_Career_Progress_All_Stat_Masks.size());
    if (ImGui::Button("COMPLETE ALL CAREER TIERS", ImVec2{245.0F, 36.0F}))
        ImGui::OpenPopup("Confirm Career Progress Complete All");
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::TextDisabled("5 character-stat writes");

    if (ImGui::BeginPopupModal(
            "Confirm Career Progress Complete All",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped(
            "This will mark all %zu Career Progress tiers complete for the active online character. "
            "It does NOT set reward-claimed flags, vehicle-claim bits, RP-claim bits, cash-claim bits, or special reward claims.",
            GTA_Career_Progress_Completion_Bit_Count);
        ImGui::Spacing();
        ImGui::BeginDisabled(busy || queueFree < GTA_Career_Progress_All_Stat_Masks.size());
        if (ImGui::Button("CONFIRM COMPLETE", ImVec2{180.0F, 34.0F})) {
            QueueAllCareerProgress();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("CANCEL", ImVec2{110.0F, 34.0F}))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    DrawDivider();

    constexpr auto tableFlags =
        ImGuiTableFlags_BordersInnerH |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_SizingStretchProp;
    if (ImGui::BeginTable(
            "##CareerProgressTable",
            5,
            tableFlags,
            ImVec2{0.0F, 430.0F})) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("CAREER", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("STAT", ImGuiTableColumnFlags_WidthFixed, 74.0F);
        ImGui::TableSetupColumn("TIERS", ImGuiTableColumnFlags_WidthFixed, 64.0F);
        ImGui::TableSetupColumn("BITS", ImGuiTableColumnFlags_WidthFixed, 100.0F);
        ImGui::TableSetupColumn("ACTION", ImGuiTableColumnFlags_WidthFixed, 108.0F);
        ImGui::TableHeadersRow();

        for (const auto& entry : GTA_Career_Progress_Entries) {
            ImGui::PushID(entry.careerId);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(entry.name.data(), entry.name.data() + entry.name.size());
            ImGui::TextDisabled(
                "%.*s",
                static_cast<int>(entry.internalTag.size()),
                entry.internalTag.data());

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%d", entry.statIndex);

            ImGui::TableSetColumnIndex(2);
            const auto slot = CareerFieldSlot(entry.statIndex);
            if (CareerState().snapshotValid && slot < CareerState().fieldValues.size()) {
                const auto tiers = GTACareerProgressCompletedTierCount(
                    entry,
                    CareerState().fieldValues[slot]);
                ImGui::Text(
                    "%zu/%zu",
                    tiers,
                    GTA_Career_Progress_Tiers_Per_Career);
            } else {
                ImGui::TextDisabled("--/4");
            }

            ImGui::TableSetColumnIndex(3);
            ImGui::Text("0x%08X", static_cast<unsigned int>(entry.completionMask));

            ImGui::TableSetColumnIndex(4);
            ImGui::BeginDisabled(busy || queueFree == 0);
            if (ImGui::Button("COMPLETE", ImVec2{92.0F, 28.0F}))
                QueueCareerProgress(entry.name, entry);
            ImGui::EndDisabled();
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::TextDisabled(
        "Tier status is a read-only snapshot. Career writes mark it stale until the five raw fields are refreshed again.");
    ImGui::TextDisabled(
        "Reward-claim state is intentionally untouched so Career rewards are not falsely marked as already collected.");
    ImGui::TextDisabled(
        "Stat 18532 preserves every bit above the verified Career mask 0x0000FFFF.");

    DrawDivider();
    DrawCareerProgressStatus();
}

void DrawHalloweenPresetRow(
    std::string_view name,
    std::string_view description,
    std::size_t count,
    bool batchActive,
    std::vector<std::int32_t> (*buildIndices)())
{
    ImGui::PushID(name.data());
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(name.data(), name.data() + name.size());
    ImGui::TextDisabled(
        "%.*s",
        static_cast<int>(description.size()),
        description.data());

    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%zu", count);

    ImGui::TableSetColumnIndex(2);
    ImGui::BeginDisabled(batchActive);
    if (ImGui::Button("APPLY", ImVec2{92.0F, 28.0F}))
        QueuePackedBoolUnlocks(name, buildIndices());
    ImGui::EndDisabled();
    ImGui::PopID();
}

void DrawHalloweenUnlocks()
{
    const auto batch = GTA_Packed_Stats_State::Instance().BatchSnapshot();
    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();

    ImGui::TextColored(palette.bronze, "HALLOWEEN UNLOCKS / PROGRESS");
    ImGui::TextWrapped(
        "Verified collectible packed BOOL paths traced from %.*s. These presets mark event "
        "collection progress; they do not force live seasonal tunables, spawns, or reward transactions.",
        static_cast<int>(GTA_Halloween_Unlock_Source.size()),
        GTA_Halloween_Unlock_Source.data());
    ImGui::Spacing();

    ImGui::BeginDisabled(batch.active);
    if (ImGui::Button("APPLY ALL HALLOWEEN FLAGS", ImVec2{230.0F, 36.0F}))
        ImGui::OpenPopup("Confirm Halloween Unlock All");
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::TextDisabled("%zu verified packed BOOLs", GTA_Halloween_Verified_Unique_Count);

    if (ImGui::BeginPopupModal(
            "Confirm Halloween Unlock All",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped(
            "This will set %zu verified Halloween collectible packed BOOLs to true for the active "
            "online character. Jack O' Lantern progress is event-scoped and may be reset by GTA's "
            "daily/seasonal event logic.",
            GTA_Halloween_Verified_Unique_Count);
        ImGui::Spacing();
        ImGui::BeginDisabled(batch.active);
        if (ImGui::Button("CONFIRM APPLY", ImVec2{170.0F, 34.0F})) {
            QueuePackedBoolUnlocks(
                "All Halloween Progress",
                GTAAllVerifiedHalloweenUnlockIndices());
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("CANCEL", ImVec2{110.0F, 34.0F}))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    constexpr auto tableFlags =
        ImGuiTableFlags_BordersInnerH |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_SizingStretchProp;

    if (ImGui::BeginTable("##HalloweenUnlockTable", 3, tableFlags)) {
        ImGui::TableSetupColumn("EVENT", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("BOOLS", ImGuiTableColumnFlags_WidthFixed, 72.0F);
        ImGui::TableSetupColumn("ACTION", ImGuiTableColumnFlags_WidthFixed, 108.0F);
        ImGui::TableHeadersRow();

        DrawHalloweenPresetRow(
            "Jack O' Lanterns",
            "200 Halloween pumpkin location flags (event progress).",
            GTA_Halloween_Jack_O_Lantern_Count,
            batch.active,
            &GTAHalloweenJackOLanternIndices);
        DrawHalloweenPresetRow(
            "Ghosts Exposed 2023",
            "Ten original Ghosts Exposed photo progress flags.",
            GTA_Halloween_Ghosts_Exposed_2023_Count,
            batch.active,
            &GTAHalloweenGhostsExposed2023Indices);
        DrawHalloweenPresetRow(
            "Ghosts Exposed 2024",
            "Ten 2024 Ghosts Exposed photo progress flags.",
            GTA_Halloween_Ghosts_Exposed_2024_Count,
            batch.active,
            &GTAHalloweenGhostsExposed2024Indices);
        DrawHalloweenPresetRow(
            "Ghosts Exposed 2025",
            "Ten 2025 Ghosts Exposed photo progress flags.",
            GTA_Halloween_Ghosts_Exposed_2025_Count,
            batch.active,
            &GTAHalloweenGhostsExposed2025Indices);

        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::TextDisabled(
        "2026-only Ghosts Exposed flags are intentionally excluded from this target-build preset.");
    ImGui::TextDisabled(
        "UFOs, Slashers, Phantom Car, Cerberus, peyote effects, and Halloween weather are event state, not persistent unlocks.");
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
            QueuePackedBoolUnlocks(
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
            QueuePackedBoolUnlocks(
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
    DrawClothingUnlockTable(batch.active);

    ImGui::Spacing();
    ImGui::TextDisabled(
        "Sparse reward lists are preserved; unrelated Car Meet and Arena packed stats are not written.");
    ImGui::TextDisabled(
        "Seasonal Valentine and Halloween storefront availability still depends on live tunables.");
}

void DrawCollectionUnlocks()
{
    const auto batch = GTA_Packed_Stats_State::Instance().BatchSnapshot();
    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();

    ImGui::TextColored(palette.bronze, "DAILY & COLLECTION PROGRESS");
    ImGui::TextWrapped(
        "Verified packed BOOLs sourced from %.*s. These presets mark collection/daily completion "
        "state only; GTA can reset daily-scoped flags during normal rotations.",
        static_cast<int>(GTA_Collection_Unlock_Source.size()),
        GTA_Collection_Unlock_Source.data());
    ImGui::Spacing();

    ImGui::BeginDisabled(batch.active);
    if (ImGui::Button("MARK ALL COLLECTIONS COMPLETE", ImVec2{275.0F, 36.0F}))
        ImGui::OpenPopup("Confirm Collection Unlock All");
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::TextDisabled("%zu verified packed BOOLs", GTA_Collection_Unlock_Unique_Count);

    if (ImGui::BeginPopupModal(
            "Confirm Collection Unlock All",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped(
            "This will set %zu verified daily/collection packed BOOLs to true for the active online "
            "character. Daily flags may be reset later by GTA's normal rotation logic.",
            GTA_Collection_Unlock_Unique_Count);
        ImGui::Spacing();
        ImGui::BeginDisabled(batch.active);
        if (ImGui::Button("CONFIRM APPLY", ImVec2{170.0F, 34.0F})) {
            QueuePackedBoolUnlocks(
                "All Daily & Collection Progress",
                GTAAllCollectionUnlockIndices());
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("CANCEL", ImVec2{110.0F, 34.0F}))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    constexpr auto tableFlags =
        ImGuiTableFlags_BordersInnerH |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_SizingStretchProp;

    if (ImGui::BeginTable(
            "##CollectionUnlockTable",
            3,
            tableFlags,
            ImVec2{0.0F, 430.0F})) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("COLLECTION", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("BOOLS", ImGuiTableColumnFlags_WidthFixed, 72.0F);
        ImGui::TableSetupColumn("ACTION", ImGuiTableColumnFlags_WidthFixed, 108.0F);
        ImGui::TableHeadersRow();

        for (const auto& group : GTA_Collection_Unlock_Groups) {
            ImGui::PushID(group.name.data());
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(group.name.data(), group.name.data() + group.name.size());
            ImGui::TextDisabled(
                "%.*s",
                static_cast<int>(group.description.size()),
                group.description.data());

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%zu", group.indices.size());

            ImGui::TableSetColumnIndex(2);
            ImGui::BeginDisabled(batch.active);
            if (ImGui::Button("APPLY", ImVec2{92.0F, 28.0F}))
                QueuePackedBoolUnlocks(group.name, GTACollectionUnlockIndices(group));
            ImGui::EndDisabled();
            ImGui::PopID();
        }

        ImGui::EndTable();
    }
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
    ImGui::TextColored(ImVec4{0.42F, 0.78F, 0.42F, 1.0F}, "[PROGRESSION + PRESETS READY]");

    ImGui::TextWrapped(
        "Recovery owns curated progression and unlock presets. Reads and writes are queued onto the GTA "
        "game thread through the Enhanced packed-stat and regular-stat runtimes.");
    ImGui::Spacing();

    if (ImGui::BeginTabBar("##RecoveryPageTabs")) {
        if (ImGui::BeginTabItem("PROGRESSION")) {
            DrawCareerProgression();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("UNLOCKS")) {
            DrawCharacterUnlocks();
            DrawDivider();
            DrawHalloweenUnlocks();
            DrawDivider();
            DrawClothingUnlocks();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("COLLECTIONS")) {
            DrawCollectionUnlocks();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("ACTIVITY")) {
            ImGui::TextColored(palette.bronze, "RECOVERY ACTIVITY");
            DrawDivider();
            ImGui::TextColored(palette.bronze, "CAREER TIER SNAPSHOT");
            DrawCareerRefreshStatus();
            DrawDivider();
            ImGui::TextColored(palette.bronze, "CAREER PROGRESS WRITES");
            DrawCareerProgressStatus();
            DrawDivider();
            ImGui::TextColored(palette.bronze, "PACKED UNLOCK BATCH");
            DrawUnlockBatchStatus();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}
}
