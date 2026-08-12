#pragma once

#include "Frontend/Menu/Themes/Menu_Theme.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Outfit_Editor_Extension.hpp"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cstddef>

namespace Devilz::Frontend::Self
{
namespace OutfitEditorUiDetail
{
using namespace Integrations::GTA5_Enhanced;

constexpr std::array<const char*, GTA_Outfit_Component_Count> ComponentLabels{
    "Face",
    "Mask",
    "Hair",
    "Torso",
    "Legs",
    "Bag / Parachute",
    "Shoes",
    "Accessories",
    "Undershirt",
    "Armor",
    "Decals",
    "Tops"
};

constexpr std::array<const char*, GTA_Outfit_Prop_Count> PropLabels{
    "Hats",
    "Glasses",
    "Ears",
    "Watches",
    "Bracelets"
};

struct EditorCache
{
    std::uint64_t generation = static_cast<std::uint64_t>(-1);
    std::array<GTA_Outfit_Item_State, GTA_Outfit_Component_Count> components{};
    std::array<GTA_Outfit_Item_State, GTA_Outfit_Prop_Count> props{};
};

inline EditorCache& Cache() noexcept
{
    static EditorCache cache;
    return cache;
}

inline void SyncCache(const GTA_Outfit_Editor_Snapshot& snapshot) noexcept
{
    auto& cache = Cache();
    if (cache.generation == snapshot.generation)
        return;

    cache.generation = snapshot.generation;
    cache.components = snapshot.components;
    cache.props = snapshot.props;
}

inline void DrawComponents(const GTA_Outfit_Editor_Snapshot& snapshot)
{
    auto& cache = Cache();
    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();

    ImGui::TextColored(palette.bronze, "CLOTHING COMPONENTS");
    ImGui::TextDisabled("Drawable and texture ranges are read from the current ped. Palette is preserved automatically.");

    constexpr ImGuiTableFlags flags = ImGuiTableFlags_BordersInnerV |
        ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_SizingStretchProp;

    if (!ImGui::BeginTable("##OutfitComponents", 4, flags))
        return;

    ImGui::TableSetupColumn("Slot", ImGuiTableColumnFlags_WidthStretch, 1.8F);
    ImGui::TableSetupColumn("Drawable", ImGuiTableColumnFlags_WidthStretch, 1.4F);
    ImGui::TableSetupColumn("Texture", ImGuiTableColumnFlags_WidthStretch, 1.4F);
    ImGui::TableSetupColumn("Palette", ImGuiTableColumnFlags_WidthStretch, 0.7F);
    ImGui::TableHeadersRow();

    for (std::size_t index = 0; index < cache.components.size(); ++index) {
        auto& item = cache.components[index];
        ImGui::PushID(item.slot);
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(ComponentLabels[index]);
        ImGui::SameLine();
        ImGui::TextDisabled("(%d)", item.slot);

        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(-1.0F);
        int drawable = item.drawable;
        if (ImGui::InputInt("##Drawable", &drawable, 1, 10)) {
            drawable = std::clamp(drawable, 0, (std::max)(0, item.drawableMax));
            item.drawable = drawable;
            item.texture = 0;
            RequestOutfitComponentChange(item.slot, item.drawable, item.texture, item.palette);
        }
        ImGui::TextDisabled("0 - %d", (std::max)(0, item.drawableMax));

        ImGui::TableSetColumnIndex(2);
        ImGui::SetNextItemWidth(-1.0F);
        int texture = item.texture;
        if (ImGui::InputInt("##Texture", &texture, 1, 5)) {
            texture = std::clamp(texture, 0, (std::max)(0, item.textureMax));
            item.texture = texture;
            RequestOutfitComponentChange(item.slot, item.drawable, item.texture, item.palette);
        }
        ImGui::TextDisabled("0 - %d", (std::max)(0, item.textureMax));

        ImGui::TableSetColumnIndex(3);
        ImGui::Text("%d", item.palette);

        if (!item.valid) {
            ImGui::TableSetColumnIndex(0);
            ImGui::TextDisabled("range unavailable");
        }

        ImGui::PopID();
    }

    ImGui::EndTable();
}

inline void DrawProps(const GTA_Outfit_Editor_Snapshot& snapshot)
{
    auto& cache = Cache();
    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();

    ImGui::TextColored(palette.bronze, "PROPS");
    ImGui::TextDisabled("Set Drawable to -1 to remove a prop from that slot.");

    constexpr ImGuiTableFlags flags = ImGuiTableFlags_BordersInnerV |
        ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_SizingStretchProp;

    if (!ImGui::BeginTable("##OutfitProps", 3, flags))
        return;

    ImGui::TableSetupColumn("Slot", ImGuiTableColumnFlags_WidthStretch, 1.8F);
    ImGui::TableSetupColumn("Drawable", ImGuiTableColumnFlags_WidthStretch, 1.4F);
    ImGui::TableSetupColumn("Texture", ImGuiTableColumnFlags_WidthStretch, 1.4F);
    ImGui::TableHeadersRow();

    for (std::size_t index = 0; index < cache.props.size(); ++index) {
        auto& item = cache.props[index];
        ImGui::PushID(100 + item.slot);
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(PropLabels[index]);
        ImGui::SameLine();
        ImGui::TextDisabled("(%d)", item.slot);

        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(-1.0F);
        int drawable = item.drawable;
        if (ImGui::InputInt("##Drawable", &drawable, 1, 10)) {
            drawable = std::clamp(drawable, -1, (std::max)(-1, item.drawableMax));
            item.drawable = drawable;
            item.texture = 0;
            RequestOutfitPropChange(item.slot, item.drawable, item.texture);
        }
        ImGui::TextDisabled("-1 - %d", (std::max)(-1, item.drawableMax));

        ImGui::TableSetColumnIndex(2);
        ImGui::BeginDisabled(item.drawable < 0);
        ImGui::SetNextItemWidth(-1.0F);
        int texture = item.texture;
        if (ImGui::InputInt("##Texture", &texture, 1, 5)) {
            texture = std::clamp(texture, 0, (std::max)(0, item.textureMax));
            item.texture = texture;
            RequestOutfitPropChange(item.slot, item.drawable, item.texture);
        }
        ImGui::TextDisabled("0 - %d", (std::max)(0, item.textureMax));
        ImGui::EndDisabled();

        ImGui::PopID();
    }

    ImGui::EndTable();
}
}

inline void DrawOutfitEditorPage()
{
    using namespace Integrations::GTA5_Enhanced;
    using namespace OutfitEditorUiDetail;

    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();
    const auto snapshot = OutfitEditorSnapshot();
    SyncCache(snapshot);

    ImGui::TextColored(palette.emberRed, "OUTFIT EDITOR");
    ImGui::SameLine();
    ImGui::TextDisabled("- live local ped wardrobe");
    Themes::Menu_Theme_Manager::Instance().DrawDivider();

    if (ImGui::Button("REFRESH OUTFIT", ImVec2(180.0F, 36.0F)))
        RequestOutfitEditorRefresh();
    ImGui::SameLine();
    if (snapshot.ready)
        ImGui::TextDisabled("Ped %d | Model 0x%08X", snapshot.ped, snapshot.modelHash);
    else
        ImGui::TextDisabled("Waiting for the validated game-thread outfit snapshot...");

    if (!snapshot.ready) {
        ImGui::Spacing();
        ImGui::TextWrapped("The editor will populate automatically once the local ped and clothing native handlers are available.");
        return;
    }

    Themes::Menu_Theme_Manager::Instance().DrawDivider();
    DrawComponents(snapshot);

    Themes::Menu_Theme_Manager::Instance().DrawDivider();
    DrawProps(snapshot);

    Themes::Menu_Theme_Manager::Instance().DrawDivider();
    ImGui::TextDisabled("Changes are queued from ImGui and applied on the existing validated GTA game thread. Switching ped models refreshes the available ranges automatically.");
}
}
