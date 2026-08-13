#pragma once

#include "Frontend/Menu/Themes/Menu_Theme.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Unlock_Operations_State.hpp"

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
using Integrations::GTA5_Enhanced::GTA_Unlock_Batch_Kind;
using Integrations::GTA5_Enhanced::GTA_Unlock_Operation;
using Integrations::GTA5_Enhanced::GTA_Unlock_Operation_Type;
using Integrations::GTA5_Enhanced::GTA_Unlock_Operations_State;

struct Unlock_Item
{
    std::string id;
    std::string label;
    std::vector<GTA_Unlock_Operation> operations;
};

struct DLC_Group
{
    std::string name;
    std::vector<Unlock_Item> items;
};

struct Item_Status
{
    bool known = false;
    bool unlocked = false;
    bool queued = false;
    bool failed = false;
};

inline Unlock_Item PackedItem(std::int32_t index, std::string label = {})
{
    if (label.empty())
        label = "Packed clothing flag " + std::to_string(index);
    return {
        "packed:" + std::to_string(index),
        std::move(label),
        {GTA_Unlock_Operation::PackedBool(index)}
    };
}

inline void AddPackedRange(DLC_Group& group, std::int32_t first, std::int32_t last)
{
    for (auto index = first; index <= last; ++index)
        group.items.push_back(PackedItem(index));
}

inline void AddChristmas2018Tees(DLC_Group& group)
{
    for (std::int32_t tee = 0; tee <= 67; ++tee) {
        char label[64]{};
        std::snprintf(label, sizeof(label), "Christmas 2018 Tee %03d", tee);
        group.items.push_back(PackedItem(25032 + tee, label));
    }
}

inline const std::vector<DLC_Group>& Catalog()
{
    static const auto catalog = [] {
        std::vector<DLC_Group> groups;

        DLC_Group festive2018{"Festive Surprise 2018", {}};
        AddChristmas2018Tees(festive2018);
        groups.push_back(std::move(festive2018));

        DLC_Group valentines{"Valentine's Day", {}};
        valentines.items.push_back({
            "global:274256",
            "Valentine clothing catalog gate",
            {GTA_Unlock_Operation::TunableInt(274256, 1)}
        });
        groups.push_back(std::move(valentines));

        DLC_Group drugWars{"Los Santos Drug Wars", {}};
        AddPackedRange(drugWars, 36699, 36770);
        groups.push_back(std::move(drugWars));

        DLC_Group mercenaries{"San Andreas Mercenaries", {}};
        AddPackedRange(mercenaries, 41943, 41945);
        groups.push_back(std::move(mercenaries));

        DLC_Group chopShop{"The Chop Shop", {}};
        AddPackedRange(chopShop, 42154, 42247);
        AddPackedRange(chopShop, 42130, 42146);
        for (const auto index : std::array<std::int32_t, 6>{42111, 42153, 42055, 42152, 42063, 42119})
            chopShop.items.push_back(PackedItem(index));
        groups.push_back(std::move(chopShop));

        return groups;
    }();
    return catalog;
}

inline std::vector<GTA_Unlock_Operation> AllOperations()
{
    std::vector<GTA_Unlock_Operation> operations;
    for (const auto& group : Catalog())
        for (const auto& item : group.items)
            operations.insert(operations.end(), item.operations.begin(), item.operations.end());
    return operations;
}

inline std::size_t ItemCount()
{
    std::size_t count = 0;
    for (const auto& group : Catalog())
        count += group.items.size();
    return count;
}

inline bool ContainsInsensitive(std::string_view text, std::string_view query)
{
    if (query.empty())
        return true;
    auto lower = [](unsigned char character) { return static_cast<char>(std::tolower(character)); };
    std::string haystack(text);
    std::string needle(query);
    std::transform(haystack.begin(), haystack.end(), haystack.begin(), lower);
    std::transform(needle.begin(), needle.end(), needle.begin(), lower);
    return haystack.find(needle) != std::string::npos;
}

inline std::string OperationSearchText(const GTA_Unlock_Operation& operation)
{
    switch (operation.type) {
    case GTA_Unlock_Operation_Type::PackedBool:
        return "packed " + std::to_string(operation.index);
    case GTA_Unlock_Operation_Type::TunableInt:
        return "global tunable " + std::to_string(operation.index);
    case GTA_Unlock_Operation_Type::StatBool:
        return "stat bool " + operation.statName;
    case GTA_Unlock_Operation_Type::StatInt:
        return "stat int " + operation.statName;
    default:
        return {};
    }
}

inline bool ItemMatches(const Unlock_Item& item, std::string_view query)
{
    if (query.empty() || ContainsInsensitive(item.label, query))
        return true;
    for (const auto& operation : item.operations)
        if (ContainsInsensitive(OperationSearchText(operation), query))
            return true;
    return false;
}

inline Item_Status GetItemStatus(const Unlock_Item& item, const GTA_Unlock_Operations_State& state)
{
    Item_Status status{};
    if (item.operations.empty())
        return status;

    status.known = true;
    status.unlocked = true;
    for (const auto& operation : item.operations) {
        const auto record = state.Snapshot(operation);
        status.queued = status.queued || record.queued;
        status.failed = status.failed || record.failed;
        if (!record.known)
            status.known = false;
        if (!record.known || !record.matched)
            status.unlocked = false;
    }
    return status;
}

inline void DrawStatus(const Item_Status& status)
{
    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();
    if (status.queued) {
        ImGui::TextDisabled("Queued");
    } else if (status.failed) {
        ImGui::TextColored(palette.emberRed, "Failed");
    } else if (!status.known) {
        ImGui::TextDisabled("Unknown");
    } else if (status.unlocked) {
        ImGui::TextColored(palette.bronze, "Unlocked");
    } else {
        ImGui::TextDisabled("Locked");
    }
}

inline std::vector<GTA_Unlock_Operation> SelectedOperations(const std::unordered_set<std::string>& selected)
{
    std::vector<GTA_Unlock_Operation> operations;
    for (const auto& group : Catalog()) {
        for (const auto& item : group.items) {
            if (!selected.contains(item.id))
                continue;
            operations.insert(operations.end(), item.operations.begin(), item.operations.end());
        }
    }
    return operations;
}

inline void DrawClothingUnlocks()
{
    static std::unordered_set<std::string> selected;
    static char search[96]{};
    static bool initialRefreshQueued = false;

    auto& unlocks = GTA_Unlock_Operations_State::Instance();
    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();
    const auto allOperations = AllOperations();

    if (!initialRefreshQueued) {
        (void)unlocks.RequestRefresh(allOperations);
        initialRefreshQueued = true;
    }

    ImGui::TextColored(palette.bronze, "CLOTHING UNLOCKS");
    ImGui::TextWrapped(
        "DLC groups use a multi-gate unlock engine. One clothing checkbox can contain packed flags, normal stats, "
        "or verified script-global/tunable operations without changing the menu workflow.");
    ImGui::TextDisabled(
        "Current executable catalog: 260 verified packed clothing gates plus 1 build-guarded Enhanced global gate. Unverified mappings stay out.");

    ImGui::SetNextItemWidth(460.0F);
    ImGui::InputTextWithHint("##ClothingSearch", "Search DLC, clothing label, stat, packed index, or tunable", search, sizeof(search));

    if (ImGui::Button("SELECT ALL", ImVec2(130.0F, 34.0F))) {
        for (const auto& group : Catalog())
            for (const auto& item : group.items)
                selected.insert(item.id);
    }
    ImGui::SameLine();
    if (ImGui::Button("CLEAR", ImVec2(100.0F, 34.0F)))
        selected.clear();
    ImGui::SameLine();
    if (ImGui::Button("REFRESH STATUS", ImVec2(160.0F, 34.0F)))
        (void)unlocks.RequestRefresh(allOperations);

    const auto selectedOperations = SelectedOperations(selected);
    ImGui::SameLine();
    ImGui::BeginDisabled(selectedOperations.empty());
    if (ImGui::Button("UNLOCK SELECTED", ImVec2(180.0F, 34.0F)))
        (void)unlocks.RequestApply(selectedOperations);
    ImGui::EndDisabled();

    const auto batch = unlocks.BatchSnapshot();
    if (batch.kind != GTA_Unlock_Batch_Kind::None && batch.total != 0) {
        const float progress = static_cast<float>(batch.completed) / static_cast<float>(batch.total);
        char overlay[96]{};
        std::snprintf(
            overlay,
            sizeof(overlay),
            "%s %zu / %zu%s",
            batch.kind == GTA_Unlock_Batch_Kind::Refresh ? "Refreshing" : "Applying",
            batch.completed,
            batch.total,
            batch.failed ? " (failures)" : "");
        ImGui::ProgressBar(progress, ImVec2(-1.0F, 0.0F), overlay);
    }

    ImGui::TextDisabled("Selected: %zu / %zu clothing entries", selected.size(), ItemCount());
    Themes::Menu_Theme_Manager::Instance().DrawDivider();

    const std::string_view query(search);
    for (const auto& group : Catalog()) {
        std::size_t selectedCount = 0;
        std::size_t unlockedCount = 0;
        std::size_t knownCount = 0;
        bool groupMatches = ContainsInsensitive(group.name, query);

        for (const auto& item : group.items) {
            if (selected.contains(item.id))
                ++selectedCount;
            const auto status = GetItemStatus(item, unlocks);
            if (status.known) {
                ++knownCount;
                if (status.unlocked)
                    ++unlockedCount;
            }
            if (!groupMatches && ItemMatches(item, query))
                groupMatches = true;
        }

        if (!groupMatches)
            continue;

        ImGui::PushID(group.name.c_str());
        bool allSelected = selectedCount == group.items.size();
        if (ImGui::Checkbox("##DLCSelect", &allSelected)) {
            for (const auto& item : group.items) {
                if (allSelected)
                    selected.insert(item.id);
                else
                    selected.erase(item.id);
            }
        }
        ImGui::SameLine();
        if (selectedCount != 0 && selectedCount != group.items.size()) {
            ImGui::TextDisabled("[-]");
            ImGui::SameLine();
        }

        char header[192]{};
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
                if (!query.empty() && !ContainsInsensitive(group.name, query) && !ItemMatches(item, query))
                    continue;

                ImGui::PushID(item.id.c_str());
                bool isSelected = selected.contains(item.id);
                if (ImGui::Checkbox(item.label.c_str(), &isSelected)) {
                    if (isSelected)
                        selected.insert(item.id);
                    else
                        selected.erase(item.id);
                }

                if (ImGui::IsItemHovered() && !item.operations.empty()) {
                    const auto operation = OperationSearchText(item.operations.front());
                    if (item.operations.size() == 1)
                        ImGui::SetTooltip("%s", operation.c_str());
                    else
                        ImGui::SetTooltip("%zu verified unlock operations", item.operations.size());
                }

                ImGui::SameLine(430.0F);
                DrawStatus(GetItemStatus(item, unlocks));
                ImGui::PopID();
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
}
}
}