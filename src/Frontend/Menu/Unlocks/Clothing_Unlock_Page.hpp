#pragma once

#include "Frontend/Menu/Themes/Menu_Theme.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Packed_Stats_State.hpp"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace Devilz::Frontend::Unlocks
{
namespace Clothing
{
struct Unlock_Item
{
    std::int32_t packedIndex = 0;
    std::string label;
};

struct DLC_Group
{
    std::string name;
    std::vector<Unlock_Item> items;
};

inline void AddRange(DLC_Group& group, std::int32_t first, std::int32_t last)
{
    for (auto index = first; index <= last; ++index)
        group.items.push_back({index, "Packed clothing flag " + std::to_string(index)});
}

inline const std::vector<DLC_Group>& Catalog()
{
    static const auto catalog = [] {
        std::vector<DLC_Group> groups;

        DLC_Group drugWars{"Los Santos Drug Wars", {}};
        AddRange(drugWars, 36699, 36770);
        groups.push_back(std::move(drugWars));

        DLC_Group mercenaries{"San Andreas Mercenaries", {}};
        AddRange(mercenaries, 41943, 41945);
        groups.push_back(std::move(mercenaries));

        DLC_Group chopShop{"The Chop Shop", {}};
        AddRange(chopShop, 42154, 42247);
        AddRange(chopShop, 42130, 42146);
        for (const auto index : std::array<std::int32_t, 6>{42111, 42153, 42055, 42152, 42063, 42119})
            chopShop.items.push_back({index, "Packed clothing flag " + std::to_string(index)});
        groups.push_back(std::move(chopShop));

        return groups;
    }();
    return catalog;
}

inline std::vector<std::int32_t> AllIndices()
{
    std::vector<std::int32_t> indices;
    for (const auto& group : Catalog())
        for (const auto& item : group.items)
            indices.push_back(item.packedIndex);
    std::sort(indices.begin(), indices.end());
    indices.erase(std::unique(indices.begin(), indices.end()), indices.end());
    return indices;
}

inline bool ContainsInsensitive(std::string_view text, std::string_view query)
{
    if (query.empty())
        return true;
    auto lower = [](unsigned char c) { return static_cast<char>(std::tolower(c)); };
    std::string haystack(text);
    std::string needle(query);
    std::transform(haystack.begin(), haystack.end(), haystack.begin(), lower);
    std::transform(needle.begin(), needle.end(), needle.begin(), lower);
    return haystack.find(needle) != std::string::npos;
}

inline void DrawStatus(const Integrations::GTA5_Enhanced::GTA_Packed_Stat_Record& record)
{
    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();
    if (record.queued) {
        ImGui::TextDisabled("Queued");
    } else if (record.failed) {
        ImGui::TextColored(palette.emberRed, "Failed");
    } else if (!record.known) {
        ImGui::TextDisabled("Unknown");
    } else if (record.value) {
        ImGui::TextColored(palette.bronze, "Unlocked");
    } else {
        ImGui::TextDisabled("Locked");
    }
}

inline void DrawClothingUnlocks()
{
    using Integrations::GTA5_Enhanced::GTA_Packed_Stat_Batch_Kind;
    using Integrations::GTA5_Enhanced::GTA_Packed_Stats_State;

    static std::unordered_set<std::int32_t> selected;
    static char search[64]{};
    static bool initialRefreshQueued = false;

    auto& stats = GTA_Packed_Stats_State::Instance();
    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();
    const auto allIndices = AllIndices();

    if (!initialRefreshQueued) {
        (void)stats.RequestReads(allIndices);
        initialRefreshQueued = true;
    }

    ImGui::TextColored(palette.bronze, "CLOTHING UNLOCKS");
    ImGui::TextWrapped(
        "Verified packed clothing flags are grouped by DLC. Selection is separate from live unlock status: "
        "unchecking an item never re-locks it. Older DLCs stay hidden until their mappings are verified.");
    ImGui::TextDisabled(
        "The public mappings used here identify these flags by DLC/index, not by stable localized clothing names.");

    ImGui::SetNextItemWidth(420.0F);
    ImGui::InputTextWithHint("##ClothingSearch", "Search DLC or packed index", search, sizeof(search));

    if (ImGui::Button("SELECT ALL", ImVec2(130.0F, 34.0F)))
        selected.insert(allIndices.begin(), allIndices.end());
    ImGui::SameLine();
    if (ImGui::Button("CLEAR", ImVec2(100.0F, 34.0F)))
        selected.clear();
    ImGui::SameLine();
    if (ImGui::Button("REFRESH STATUS", ImVec2(160.0F, 34.0F)))
        (void)stats.RequestReads(allIndices);

    std::vector<std::int32_t> selectedIndices;
    selectedIndices.reserve(selected.size());
    for (const auto index : selected)
        selectedIndices.push_back(index);
    std::sort(selectedIndices.begin(), selectedIndices.end());

    ImGui::SameLine();
    ImGui::BeginDisabled(selectedIndices.empty());
    if (ImGui::Button("UNLOCK SELECTED", ImVec2(180.0F, 34.0F)))
        (void)stats.RequestWrites(selectedIndices, true);
    ImGui::EndDisabled();

    const auto batch = stats.BatchSnapshot();
    if (batch.kind != GTA_Packed_Stat_Batch_Kind::None && batch.total != 0) {
        const float progress = static_cast<float>(batch.completed) / static_cast<float>(batch.total);
        char overlay[96]{};
        std::snprintf(
            overlay,
            sizeof(overlay),
            "%s %zu / %zu%s",
            batch.kind == GTA_Packed_Stat_Batch_Kind::Refresh ? "Refreshing" : "Applying",
            batch.completed,
            batch.total,
            batch.failed ? " (failures)" : "");
        ImGui::ProgressBar(progress, ImVec2(-1.0F, 0.0F), overlay);
    }

    ImGui::TextDisabled("Selected: %zu / %zu", selected.size(), allIndices.size());
    Themes::Menu_Theme_Manager::Instance().DrawDivider();

    const std::string_view query(search);
    for (const auto& group : Catalog()) {
        std::size_t selectedCount = 0;
        std::size_t unlockedCount = 0;
        std::size_t knownCount = 0;
        bool groupMatches = ContainsInsensitive(group.name, query);

        for (const auto& item : group.items) {
            if (selected.contains(item.packedIndex))
                ++selectedCount;
            const auto record = stats.Snapshot(item.packedIndex);
            if (record.known) {
                ++knownCount;
                if (record.value)
                    ++unlockedCount;
            }
            if (!groupMatches && ContainsInsensitive(std::to_string(item.packedIndex), query))
                groupMatches = true;
        }

        if (!groupMatches)
            continue;

        ImGui::PushID(group.name.c_str());
        bool allSelected = selectedCount == group.items.size();
        if (ImGui::Checkbox("##DLCSelect", &allSelected)) {
            for (const auto& item : group.items) {
                if (allSelected)
                    selected.insert(item.packedIndex);
                else
                    selected.erase(item.packedIndex);
            }
        }
        ImGui::SameLine();
        if (selectedCount != 0 && selectedCount != group.items.size()) {
            ImGui::TextDisabled("[-]");
            ImGui::SameLine();
        }

        char header[160]{};
        std::snprintf(
            header,
            sizeof(header),
            "%s   %zu / %zu unlocked%s",
            group.name.c_str(),
            unlockedCount,
            group.items.size(),
            knownCount == group.items.size() ? "" : " (status incomplete)");

        const bool open = ImGui::TreeNodeEx("##DLCItems", ImGuiTreeNodeFlags_SpanAvailWidth, "%s", header);
        if (open) {
            for (const auto& item : group.items) {
                if (!query.empty() &&
                    !ContainsInsensitive(group.name, query) &&
                    !ContainsInsensitive(std::to_string(item.packedIndex), query)) {
                    continue;
                }

                ImGui::PushID(item.packedIndex);
                bool isSelected = selected.contains(item.packedIndex);
                if (ImGui::Checkbox(item.label.c_str(), &isSelected)) {
                    if (isSelected)
                        selected.insert(item.packedIndex);
                    else
                        selected.erase(item.packedIndex);
                }
                ImGui::SameLine(430.0F);
                DrawStatus(stats.Snapshot(item.packedIndex));
                ImGui::PopID();
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
}
}
}
