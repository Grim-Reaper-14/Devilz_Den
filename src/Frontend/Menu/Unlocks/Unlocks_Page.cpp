#include "Unlocks_Page.hpp"

#include "Frontend/Menu/Themes/Menu_Theme.hpp"

#include <imgui.h>

namespace Devilz::Frontend::Unlocks
{
namespace
{
void UnlockSection(const char* title, const char* detail)
{
    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();
    ImGui::TextColored(palette.bronze, "%s", title);
    ImGui::TextWrapped("%s", detail);
    ImGui::Spacing();
}
}

void DrawUnlocksPage()
{
    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();

    ImGui::TextColored(palette.emberRed, "UNLOCKS");
    ImGui::SameLine();
    ImGui::TextDisabled("- progression, awards and content unlocks");
    Themes::Menu_Theme_Manager::Instance().DrawDivider();

    ImGui::TextWrapped(
        "Unlock tools live in this domain. Runtime actions will only be enabled as their GTA V Enhanced "
        "stat/native mappings are verified for the supported build.");

    Themes::Menu_Theme_Manager::Instance().DrawDivider();
    UnlockSection("PROGRESSION", "Rank, progression and character milestone unlocks.");
    UnlockSection("AWARDS", "Awards, achievements and challenge completion flags.");
    UnlockSection("CLOTHING", "Outfits, clothing items, masks and appearance unlocks.");
    UnlockSection("VEHICLES", "Vehicle availability, trade-price and related content unlocks.");
    UnlockSection("WEAPONS", "Weapon, component and equipment unlock flags.");
    UnlockSection("HEISTS", "Heist access, setup progression and associated unlock states.");
    UnlockSection("MISCELLANEOUS", "Additional verified unlockable content and progression flags.");

    Themes::Menu_Theme_Manager::Instance().DrawDivider();
    ImGui::TextDisabled("No unlock writes are performed by this page yet.");
}
}
