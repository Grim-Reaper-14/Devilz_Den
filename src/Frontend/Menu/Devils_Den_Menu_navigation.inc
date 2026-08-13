#include "Frontend/Menu/Menu_Appearance.hpp"

namespace Devilz::Frontend
{
void Devils_Den_Menu::DrawNavigation()
{
    auto& appearance = Menu_Appearance_State::Instance();
    auto& theme = Themes::Menu_Theme_Manager::Instance();
    const auto& palette = theme.Palette();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, palette.deepStone);
    ImGui::TextColored(palette.bronze, "CATEGORIES");
    MedievalDivider();

    struct NavigationEntry
    {
        const char* label;
        Page page;
        Themes::Menu_Icon icon;
    };
    constexpr std::array entries{
        NavigationEntry{"       SELF", Page::Self, Themes::Menu_Icon::Self},
        NavigationEntry{"       WEAPONS", Page::Weapons, Themes::Menu_Icon::Weapons},
        NavigationEntry{"       VEHICLE", Page::Vehicle, Themes::Menu_Icon::Vehicle},
        NavigationEntry{"       TELEPORT", Page::Teleport, Themes::Menu_Icon::Teleport},
        NavigationEntry{"       WORLD", Page::World, Themes::Menu_Icon::World},
        NavigationEntry{"       NETWORK", Page::Network, Themes::Menu_Icon::Network},
        NavigationEntry{"       DEFENSE", Page::Defense, Themes::Menu_Icon::Network},
        NavigationEntry{"       LUA", Page::Lua, Themes::Menu_Icon::Lua},
        NavigationEntry{"       SETTINGS", Page::Settings, Themes::Menu_Icon::Settings}
    };

    for (const auto& entry : entries) {
        const bool selected = m_page == entry.page;
        const ImVec2 itemMin = ImGui::GetCursorScreenPos();
        const float itemWidth = ImGui::GetContentRegionAvail().x;
        auto* foreground = ImGui::GetForegroundDrawList();
        theme.DrawOrnateFrame(foreground, itemMin, ImVec2{itemMin.x + itemWidth, itemMin.y + 42.0F}, selected);
        const ImVec2 iconCenter{itemMin.x + 24.0F, itemMin.y + 21.0F};
        if (!appearance.DrawCustomIcon(entry.icon, foreground, iconCenter, 12.0F)) {
            theme.DrawIcon(entry.icon, foreground, iconCenter, 10.5F,
                ImGui::GetColorU32(selected ? palette.emberRed : palette.bronze));
        }

        ImGui::PushStyleColor(ImGuiCol_Button,
            selected ? ImVec4{0.38F, 0.055F, 0.035F, 1.00F} : palette.iron);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
            selected ? ImVec4{0.48F, 0.070F, 0.042F, 1.00F} : palette.ironLight);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,
            selected ? ImVec4{0.55F, 0.075F, 0.042F, 1.00F} : palette.stoneLight);
        ImGui::PushStyleColor(ImGuiCol_Text, selected ? palette.parchment : palette.bronze);
        if (ImGui::Button(entry.label, ImVec2(itemWidth, 42.0F)))
            m_page = entry.page;
        ImGui::PopStyleColor(4);
        ImGui::Dummy(ImVec2(0.0F, 4.0F));
    }

    ImGui::PopStyleColor();
}
}
