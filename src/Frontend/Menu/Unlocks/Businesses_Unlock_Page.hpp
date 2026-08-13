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
#include <utility>
#include <vector>

namespace Devilz::Frontend::Unlocks
{
namespace Businesses
{
using Integrations::GTA5_Enhanced::GTA_Unlock_Batch_Kind;
using Integrations::GTA5_Enhanced::GTA_Unlock_Operation;
using Integrations::GTA5_Enhanced::GTA_Unlock_Operation_Type;
using Integrations::GTA5_Enhanced::GTA_Unlock_Operations_State;

struct Unlock_Item
{
    std::string id;
    std::string label;
    GTA_Unlock_Operation operation;
    bool refreshStatus = true;
};

struct Unlock_Group
{
    std::string name;
    std::vector<Unlock_Item> items;
};

struct Research_Definition
{
    std::int32_t packedIndex;
    const char* label;
};

inline constexpr std::array<Research_Definition, 51> BunkerResearch{{
    {15381, "APC SAM Battery"},
    {15382, "Ballistic Equipment"},
    {15428, "Half-track 20mm Quad Autocannon"},
    {15429, "Weaponized Tampa Dual Remote Minigun"},
     {15430, "Weaponized Tampa Rear-Firing Mortar"},
    {15431, "Weaponized Tampa Front Missile Launchers"},
    {15491, "Weaponized Tampa Heavy Chassis Upgrade"},
    {15432, "Dune FAV 40mm Grenade Launcher"},
    {15433, "Dune FAV 7.62mm Minigun"},
    {15434, "Insurgent Pick-Up Custom .50 Cal Minigun"},
    {15435, "Insurgent Pick-Up Custom Heavy Armor Plating"},
    {15436, "Technical Custom 7.62mm Minigun"},
    {15437, "Technical Custom Ram-bar"},
    {15438, "Technical Custom Brute-bar"},
    {15439, "Technical Custom Heavy Chassis Upgrade"},
    {15447, "Oppressor Missile Launchers"},
    {15448, "Fractal Livery Set"},
    {15449, "Digital Livery Set"},
    {15450, "Geometric Livery Set"},
    {15451, "Nature Reserve Livery Set"},
    {15452, "Naval Battle Livery Set"},
    {15453, "Anti-Aircraft Trailer Dual 20mm Flak"},
    {15454, "Anti-Aircraft Trailer Homing Missile Battery"},
    {15455, "Mobile Operations Center Rear Turrets"},
    {15456, "Incendiary Rounds"},
    {15457, "Hollow Point Rounds"},
    {15458, "Armor Piercing Rounds"},
    {15459, "Full Metal Jacket Rounds"},
    {15460, "Explosive Rounds"},
    {15461, "Pistol Mk II Mounted Scope"},
    {15462, "Pistol Mk II Compensator"},
    {15463, "SMG Mk II Holographic Sight"},
    {15464, "SMG Mk II Heavy Barrel"},
    {15465, "Heavy Sniper Mk II Night Vision Scope"},
    {15466, "Heavy Sniper Mk II Thermal Scope"},
    {15467, "Heavy Sniper Mk II Heavy Barrel"},
    {15468, "Combat MG Mk II Holographic Sight"},
    {15469, "Combat MG Mk II Heavy Barrel"},
    {15470, "Assault Rifle Mk II Holographic Sight"},
    {15471, "Assault Rifle Mk II Heavy Barrel"},
    {15472, "Carbine Rifle Mk II Holographic Sight"},
    {15473, "Carbine Rifle Mk II Heavy Barrel"},
    {15474, "Proximity Mines"},
    {15492, "Brushstroke Camo Mk II Weapon Livery"},
    {15493, "Skull Mk II Weapon Livery"},
    {15494, "Sessanta Nove Mk II Weapon Livery"},
    {15495, "Perseus Mk II Weapon Livery"},
    {15496, "Leopard Mk II Weapon Livery"},
    {15497, "Zebra Mk II Weapon Livery"},
    {15498, "Geometric Mk II Weapon Livery"},
    {15499, "Boom! Mk II Weapon Livery"}
}};

inline Unlock_Item PackedItem(std::int32_t packedIndex, std::string label)
{
    return {
        "packed:" + std::to_string(packedIndex),
        std::move(label),
        GTA_Unlock_Operation::PackedBool(packedIndex),
        true
    };
}

inline Unlock_Item StatIntItem(std::string statName, std::string label, std::int32_t value, bool refreshStatus)
{
    const auto id = "stat-int:" + statName;
    return {
        id,
        std::move(label),
        GTA_Unlock_Operation::StatInt(std::move(statName), value),
        refreshStatus
    };
}

inline Unlock_Item StatBoolItem(std::string statName, std::string label)
{
    const auto id = "stat-bool:" + statName;
    return {
        id,
        std::move(label),
        GTA_Unlock_Operation::StatBool(std::move(statName)),
        true
    };
}

inline const std::vector<Unlock_Group>& BunkerCatalog()
{
    static const auto catalog = [] {
        std::vector<Unlock_Group> groups;

        Unlock_Group research{"Research Projects", {}};
        research.items.reserve(BunkerResearch.size());
        for (const auto& definition : BunkerResearch)
            research.items.push_back(PackedItem(definition.packedIndex, definition.label));
        groups.push_back(std::move(research));

        Unlock_Group shootingRange{"Shooting Range", {}};
        shootingRange.items.push_back(StatIntItem("MPX_SR_HIGHSCORE_1", "Challenge 1 high-score target", 910, false));
        shootingRange.items.push_back(StatIntItem("MPX_SR_HIGHSCORE_2", "Challenge 2 high-score target", 2500, false));
        shootingRange.items.push_back(StatIntItem("MPX_SR_HIGHSCORE_3", "Challenge 3 high-score target", 3440, false));
        shootingRange.items.push_back(StatIntItem("MPX_SR_HIGHSCORE_4", "Challenge 4 high-score target", 3340, false));
        shootingRange.items.push_back(StatIntItem("MPX_SR_HIGHSCORE_5", "Challenge 5 high-score target", 4150, false));
        shootingRange.items.push_back(StatIntItem("MPX_SR_HIGHSCORE_6", "Challenge 6 high-score target", 560, false));
        shootingRange.items.push_back(StatIntItem("MPX_SR_TARGETS_HIT", "Targets-hit completion target", 270, false));
        shootingRange.items.push_back(StatIntItem("MPX_SR_WEAFON_BIT_SET", "All shooting-range weapon challenge bits", 262143, true));
        shootingRange.items.push_back(StatBoolItem("MPX_SR_TIER_1_REWARD", "Tier 1 shooting-range reward"));
        shootingRange.items.push_back(StatBoolItem("MPX_SR_INCREASE_THROW_CAP", "Increased throwable capacity reward"));
        shootingRange.items.push_back(StatBoolItem("MPX_SR_TIER_3_REWARD", "Tier 3 shooting-range reward"));
        groups.push_back(std::move(shootingRange));

        return groups;
    }();
    return catalog;
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
    case GTA_Unlock_Operation_Type::StatBool:
        return "stat bool " + operation.statName;
    case GTA_Unlock_Operation_Type::StatInt:
        return "stat int " + operation.statName + " " + std::to_string(operation.intValue);
    case GTA_Unlock_Operation_Type::TunableInt:
        return "global tunable " + std::to_string(operation.index);
    default:
        return {};
    }
}

inline bool ItemMatches(const Unlock_Item& item, std::string_view query)
{
    return query.empty() || ContainsInsensitive(item.label, query) ||
        ContainsInsensitive(OperationSearchText(item.operation), query);
}

inline std::vector<GTA_Unlock_Operation> RefreshOperations()
{
    std::vector<GTA_Unlock_Operation> operations;
    for (const auto& group : BunkerCatalog()) {
        for (const auto& item : group.items) {
            if (item.refreshStatus)
                operations.push_back(item.operation);
        }
    }
    return operations;
}

inline std::vector<GTA_Unlock_Operation> SelectedOperations(const std::unordered_set<std::string>& selected)
{
    std::vector<GTA_Unlock_Operation> operations;
    for (const auto& group : BunkerCatalog()) {
        for (const auto& item : group.items) {
            if (selected.contains(item.id))
                operations.push_back(item.operation);
        }
    }
    return operations;
}

inline std::size_t ItemCount()
{
    std::size_t count = 0;
    for (const auto& group : BunkerCatalog())
        count += group.items.size();
    return count;
}

inline void DrawItemStatus(const Unlock_Item& item, const GTA_Unlock_Operations_State& unlocks)
{
    if (!item.refreshStatus) {
        ImGui::TextDisabled("Write target");
        return;
    }

    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();
    const auto record = unlocks.Snapshot(item.operation);
    if (record.queued) {
        ImGui::TextDisabled("Queued");
    } else if (record.known && record.matched) {
        ImGui::TextColored(palette.bronze, "Unlocked");
    } else if (record.failed) {
        ImGui::TextColored(palette.emberRed, "Failed");
    } else if (!record.known) {
        ImGui::TextDisabled("Unknown");
    } else {
        ImGui::TextDisabled("Locked");
    }
}

inline void DrawBunkerUnlocks()
{
    static std::unordered_set<std::string> selected;
    static char search[96]{};
    static bool initialRefreshQueued = false;

    auto& unlocks = GTA_Unlock_Operations_State::Instance();
    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();
    const auto refreshOperations = RefreshOperations();

    if (!initialRefreshQueued) {
        (void)unlocks.RequestRefresh(refreshOperations);
        initialRefreshQueued = true;
    }

    ImGui::TextColored(palette.bronze, "BUNKER BUSINESS");
    ImGui::TextWrapped(
        "Verified Gunrunning/Bunker unlocks are grouped into persistent research projects and Bunker shooting-range rewards. "
        "Research packed flags are mapped from the Enhanced Bunker business script.");
    ImGui::TextDisabled(
        "Catalog: 51 research projects + 11 shooting-range operations. Score counters use write targets and are not treated as exact unlock-state checks.");

    ImGui::SetNextItemWidth(460.0F);
    ImGui::InputTextWithHint(
        "##BunkerSearch",
        "Search Bunker unlock, stat name, or packed index",
        search,
        sizeof(search));

    if (ImGui::Button("SELECT ALL", ImVec2(130.0F, 34.0F))) {
        for (const auto& group : BunkerCatalog())
            for (const auto& item : group.items)
                selected.insert(item.id);
    }
    ImGui::SameLine();
    if (ImGui::Button("CLEAR", ImVec2(100.0F, 34.0F)))
        selected.clear();
    ImGui::SameLine();
    if (ImGui::Button("REFRESH STATUS", ImVec2(160.0F, 34.0F)))
        (void)unlocks.RequestRefresh(refreshOperations);

    const auto selectedOperations = SelectedOperations(selected);
    ImGui::SameLine();
    ImGui::BeginDisabled(selectedOperations.empty());
    if (ImGui::Button("UNLOCK SELECTED", ImVec2(180.0F, 34.0F)))
        (void)unlocks.RequestApply(selectedOperations);
    ImGui::EndDisabled();

    if (ImGui::Button("UNLOCK ALL BUNKER", ImVec2(200.0F, 34.0F))) {
        std::unordered_set<std::string> all;
        for (const auto& group : BunkerCatalog())
            for (const auto& item : group.items)
                all.insert(item.id);
        (void)unlocks.RequestApply(SelectedOperations(all));
    }

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

    ImGui::TextDisabled("Selected: %zu / %zu Bunker entries", selected.size(), ItemCount());
    Themes::Menu_Theme_Manager::Instance().DrawDivider();

    const std::string_view query(search);
    for (const auto& group : BunkerCatalog()) {
        bool groupMatches = ContainsInsensitive(group.name, query);
        std::size_t selectedCount = 0;
        for (const auto& item : group.items) {
            if (selected.contains(item.id))
                ++selectedCount;
            if (!groupMatches && ItemMatches(item, query))
                groupMatches = true;
        }

        if (!groupMatches)
            continue;

        ImGui::PushID(group.name.c_str());
        bool allSelected = selectedCount == group.items.size();
        if (ImGui::Checkbox("##BunkerGroupSelect", &allSelected)) {
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

        char header[160]{};
        std::snprintf(header, sizeof(header), "%s   %zu entries", group.name.c_str(), group.items.size());
        const bool open = ImGui::TreeNodeEx(
            "##BunkerGroupItems",
            ImGuiTreeNodeFlags_SpanAvailWidth,
            "%s",
            header);

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

                if (ImGui::IsItemHovered()) {
                    const auto operationText = OperationSearchText(item.operation);
                    ImGui::SetTooltip("%s", operationText.c_str());
                }

                ImGui::SameLine(500.0F);
                DrawItemStatus(item, unlocks);
                ImGui::PopID();
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
}

inline void DrawBusinessesUnlocks()
{
    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();
    ImGui::TextColored(palette.bronze, "BUSINESSES");
    ImGui::TextWrapped("Business-specific unlock packs are kept separate so each property can be verified and expanded independently.");

    if (!ImGui::BeginTabBar("##BusinessesUnlockTabs"))
        return;

    if (ImGui::BeginTabItem("BUNKER")) {
        DrawBunkerUnlocks();
        ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
}
}
}
