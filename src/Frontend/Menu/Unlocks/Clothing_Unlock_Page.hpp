#pragma once

#include "Frontend/Menu/Themes/Menu_Theme.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Unlock_Operations_State.hpp"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace Devilz::Frontend::Unlocks
{
namespace Clothing
{
using Integrations::GTA5_Enhanced::GTA_Unlock_Batch_Kind;
using Integrations::GTA5_Enhanced::GTA_Unlock_Operation;
using Integrations::GTA5_Enhanced::GTA_Unlock_Operation_Type;
using Integrations::GTA5_Enhanced::GTA_Unlock_Operations_State;

using Unlock_Condition = std::vector<GTA_Unlock_Operation>;

struct Unlock_Item
{
    std::string id;
    std::string label;
    std::vector<GTA_Unlock_Operation> applyOperations;
    std::vector<Unlock_Condition> statusAnyOf;
    bool hasExternalStatusAlternative = false;
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

inline Unlock_Item OperationItem(std::string id, std::string label, GTA_Unlock_Operation operation)
{
    const auto statusOperation = operation;
    return {
        std::move(id),
        std::move(label),
        {std::move(operation)},
        {{statusOperation}},
        false
    };
}

inline Unlock_Item PackedItem(std::int32_t index, std::string label = {})
{
    if (label.empty())
        label = "Packed clothing flag " + std::to_string(index);
    return OperationItem(
        "packed:" + std::to_string(index),
        std::move(label),
        GTA_Unlock_Operation::PackedBool(index));
}

inline Unlock_Item PackedAnyOfItem(
    std::string id,
    std::string label,
    std::int32_t applyIndex,
    std::initializer_list<std::int32_t> statusIndices)
{
    Unlock_Item item{};
    item.id = std::move(id);
    item.label = std::move(label);
    item.applyOperations.push_back(GTA_Unlock_Operation::PackedBool(applyIndex));
    item.statusAnyOf.reserve(statusIndices.size());
    for (const auto index : statusIndices)
        item.statusAnyOf.push_back({GTA_Unlock_Operation::PackedBool(index)});
    return item;
}

inline Unlock_Item PackedFallbackItem(std::int32_t index, std::string label)
{
    auto item = PackedItem(index, std::move(label));
    item.id = "packed-fallback:" + std::to_string(index);
    item.hasExternalStatusAlternative = true;
    return item;
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

inline void AddAfterHoursClothing(DLC_Group& group)
{
    group.items.push_back(PackedAnyOfItem(
        "after-hours:battle-clothing-000",
        "After Hours Battle Clothing 000",
        22108,
        {22108, 25006}));

    constexpr std::array<std::pair<std::int32_t, std::int32_t>, 18> earlyMappings{{
        {2, 9481},
        {3, 9470},
        {4, 9475},
        {5, 9472},
        {6, 9465},
        {7, 9463},
        {8, 9464},
        {9, 9468},
        {10, 9469},
        {11, 9479},
        {12, 9473},
        {13, 9480},
        {14, 9476},
        {15, 9477},
        {16, 9471},
        {17, 9474},
        {18, 9467},
        {19, 9478}
    }};

    for (const auto& [clothing, packedIndex] : earlyMappings) {
        char label[72]{};
        std::snprintf(label, sizeof(label), "After Hours Battle Clothing %03d", clothing);
        group.items.push_back(PackedItem(packedIndex, label));
    }

    group.items.push_back(PackedAnyOfItem(
        "after-hours:battle-clothing-020",
        "After Hours Battle Clothing 020",
        9462,
        {9462, 27085, 27084}));

    constexpr std::array<std::pair<std::int32_t, std::int32_t>, 10> lateMappings{{
        {21, 9466},
        {22, 22126},
        {23, 22127},
        {24, 22128},
        {25, 22124},
        {26, 22130},
        {27, 22125},
        {28, 22129},
        {29, 22131},
        {30, 22132}
    }};

    for (const auto& [clothing, packedIndex] : lateMappings) {
        char label[72]{};
        std::snprintf(label, sizeof(label), "After Hours Battle Clothing %03d", clothing);
        group.items.push_back(PackedItem(packedIndex, label));
    }

    for (std::int32_t clothing = 31; clothing <= 62; ++clothing) {
        char label[72]{};
        std::snprintf(label, sizeof(label), "After Hours Battle Clothing %03d", clothing);
        group.items.push_back(PackedItem(22147 + (clothing - 31), label));
    }
}

inline void AddLosSantosSummerSpecialTees(DLC_Group& group)
{
    for (std::int32_t tee = 0; tee <= 29; ++tee) {
        char label[72]{};
        std::snprintf(label, sizeof(label), "Los Santos Summer Special Tee %03d", tee);
        group.items.push_back(PackedItem(30260 + tee, label));
    }

    group.items.push_back(PackedItem(28255, "Los Santos Summer Special Tee 030"));

    for (std::int32_t tee = 31; tee <= 34; ++tee) {
        char label[72]{};
        std::snprintf(label, sizeof(label), "Los Santos Summer Special Tee %03d", tee);
        group.items.push_back(PackedFallbackItem(30254 + (tee - 31), label));
    }
}

inline void AddCayoPericoTees(DLC_Group& group)
{
    constexpr std::array<std::pair<std::int32_t, std::int32_t>, 48> mappings{{
        {30, 30533},
        {32, 30534},
        {28, 30535},
        {29, 30536},
        {31, 30537},
        {22, 30538},
        {23, 30539},
        {20, 30540},
        {21, 30541},
        {3, 30542},
        {4, 30543},
        {5, 30544},
        {6, 30545},
        {27, 30546},
        {26, 30547},
        {25, 30548},
        {24, 30549},
        {2, 30550},
        {1, 30551},
        {0, 30552},
        {7, 30553},
        {8, 30554},
        {9, 30555},
        {10, 30556},
        {11, 30557},
        {12, 30524},
        {13, 30525},
        {14, 30526},
        {15, 30527},
        {16, 30528},
        {17, 30529},
        {18, 30530},
        {19, 30531},
        {33, 30532},
        {45, 30570},
        {46, 30571},
        {48, 30568},
        {47, 30569},
        {49, 30634},
        {51, 30635},
        {53, 30636},
        {54, 30637},
        {55, 30703},
        {57, 30704},
        {59, 30700},
        {61, 30701},
        {63, 30702},
        {65, 30699}
    }};

    for (const auto& [tee, packedIndex] : mappings) {
        char label[64]{};
        std::snprintf(label, sizeof(label), "Cayo Perico Tee %03d", tee);
        group.items.push_back(PackedItem(packedIndex, label));
    }
}

inline void AddLosSantosTunersTees(DLC_Group& group)
{
    constexpr std::array<std::pair<std::int32_t, std::int32_t>, 5> progressionFallbacks{{
        {0, 31760},
        {2, 31761},
        {3, 31762},
        {5, 31763},
        {6, 31764}
    }};

    for (const auto& [tee, packedIndex] : progressionFallbacks) {
        char label[72]{};
        std::snprintf(label, sizeof(label), "Los Santos Tuners Tee %03d", tee);
        group.items.push_back(PackedFallbackItem(packedIndex, label));
    }

    constexpr std::array<std::pair<std::int32_t, std::int32_t>, 8> directMappings{{
        {8, 31768},
        {10, 31769},
        {11, 31770},
        {12, 31771},
        {13, 31772},
        {14, 31773},
        {15, 31774},
        {16, 31775}
    }};

    for (const auto& [tee, packedIndex] : directMappings) {
        char label[72]{};
        std::snprintf(label, sizeof(label), "Los Santos Tuners Tee %03d", tee);
        group.items.push_back(PackedItem(packedIndex, label));
    }
}

inline void AddCriminalEnterprisesTees(DLC_Group& group)
{
    group.items.push_back(PackedItem(34505, "Criminal Enterprises Tee 000"));
    group.items.push_back(PackedItem(34375, "Criminal Enterprises Tee 001"));
}

inline const std::vector<DLC_Group>& Catalog()
{
    static const auto catalog = [] {
        std::vector<DLC_Group> groups;

        DLC_Group valentines{"Valentine's Day", {}};
        valentines.items.push_back(OperationItem(
            "global:274256",
            "Valentine clothing catalog gate",
            GTA_Unlock_Operation::TunableInt(274256, 1)));
        groups.push_back(std::move(valentines));

        DLC_Group afterHours{"After Hours", {}};
        AddAfterHoursClothing(afterHours);
        groups.push_back(std::move(afterHours));

        DLC_Group festive2018{"Festive Surprise 2018", {}};
        AddChristmas2018Tees(festive2018);
        groups.push_back(std::move(festive2018));

        DLC_Group summerSpecial{"Los Santos Summer Special", {}};
        AddLosSantosSummerSpecialTees(summerSpecial);
        groups.push_back(std::move(summerSpecial));

        DLC_Group cayoPerico{"The Cayo Perico Heist", {}};
        AddCayoPericoTees(cayoPerico);
        groups.push_back(std::move(cayoPerico));

        DLC_Group tuners{"Los Santos Tuners", {}};
        AddLosSantosTunersTees(tuners);
        groups.push_back(std::move(tuners));

        DLC_Group criminalEnterprises{"The Criminal Enterprises", {}};
        AddCriminalEnterprisesTees(criminalEnterprises);
        groups.push_back(std::move(criminalEnterprises));

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
    for (const auto& group : Catalog()) {
        for (const auto& item : group.items) {
            for (const auto& alternative : item.statusAnyOf)
                operations.insert(operations.end(), alternative.begin(), alternative.end());
        }
    }
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

    for (const auto& operation : item.applyOperations) {
        if (ContainsInsensitive(OperationSearchText(operation), query))
            return true;
    }
    for (const auto& alternative : item.statusAnyOf) {
        for (const auto& operation : alternative) {
            if (ContainsInsensitive(OperationSearchText(operation), query))
                return true;
        }
    }
    return false;
}

inline Item_Status GetItemStatus(const Unlock_Item& item, const GTA_Unlock_Operations_State& state)
{
    Item_Status status{};
    if (item.statusAnyOf.empty())
        return status;

    bool sawAlternative = false;
    bool allAlternativesKnown = true;
    bool anyAlternativeMatched = false;

    for (const auto& alternative : item.statusAnyOf) {
        if (alternative.empty())
            continue;

        sawAlternative = true;
        bool alternativeKnown = true;
        bool alternativeMatched = true;
        for (const auto& operation : alternative) {
            const auto record = state.Snapshot(operation);
            status.queued = status.queued || record.queued;
            status.failed = status.failed || record.failed;
            if (!record.known)
                alternativeKnown = false;
            if (!record.known || !record.matched)
                alternativeMatched = false;
        }

        if (!alternativeKnown)
            allAlternativesKnown = false;
        if (alternativeKnown && alternativeMatched)
            anyAlternativeMatched = true;
    }

    if (!sawAlternative)
        return status;

    if (anyAlternativeMatched) {
        status.known = true;
        status.unlocked = true;
    } else if (allAlternativesKnown && !item.hasExternalStatusAlternative) {
        status.known = true;
        status.unlocked = false;
    }

    return status;
}

inline void DrawStatus(const Item_Status& status)
{
    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();
    if (status.queued) {
        ImGui::TextDisabled("Queued");
    } else if (status.known && status.unlocked) {
        ImGui::TextColored(palette.bronze, "Unlocked");
    } else if (status.failed) {
        ImGui::TextColored(palette.emberRed, "Failed");
    } else if (!status.known) {
        ImGui::TextDisabled("Unknown");
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
            operations.insert(operations.end(), item.applyOperations.begin(), item.applyOperations.end());
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
        "DLC groups use verified Enhanced unlock gates. Status can model Rockstar OR conditions separately from the operation the menu applies, "
        "so alternate flags and progression unlocks do not produce false Locked states.");
    ImGui::TextDisabled(
        "Current executable catalog: 423 verified packed clothing gates plus 1 build-guarded Enhanced global gate. Unverified mappings stay out.");

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

                if (ImGui::IsItemHovered() && !item.applyOperations.empty()) {
                    const auto operation = OperationSearchText(item.applyOperations.front());
                    if (item.statusAnyOf.size() > 1) {
                        ImGui::SetTooltip(
                            "Applies: %s\nStatus: any of %zu verified gates",
                            operation.c_str(),
                            item.statusAnyOf.size());
                    } else if (item.hasExternalStatusAlternative) {
                        ImGui::SetTooltip(
                            "Applies: %s\nStatus may also be satisfied by an in-game progression condition",
                            operation.c_str());
                    } else if (item.applyOperations.size() == 1) {
                        ImGui::SetTooltip("%s", operation.c_str());
                    } else {
                        ImGui::SetTooltip("%zu verified unlock operations", item.applyOperations.size());
                    }
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
