#include "Frontend/Menu/Menu_Appearance.hpp"
#include "Frontend/Menu/Devils_Den_Menu_supplemental.hpp"
#include "Frontend/Menu/Debug/Debug_Page.inc"
#include "Frontend/Menu/Defense/Defense_Page.inc"
#include "Frontend/Menu/Misc/Misc_Page.inc"
#include "Frontend/Menu/Recovery/Recovery_Page.inc"

namespace Devilz::Frontend
{
void Devils_Den_Menu::Draw(bool& open)
{
    if (!open)
        return;

    auto& appearance = Menu_Appearance_State::Instance();
    appearance.InitializeOnce();
    appearance.Tick();

    auto& theme = Themes::Menu_Theme_Manager::Instance();
    const auto& palette = theme.Palette();

    ImGui::SetNextWindowSize(ImVec2(1320.0F, 790.0F), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(appearance.BackgroundEnabled() ? 0.82F : 0.995F);
    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    if (!ImGui::Begin("##DevilsDenRoot", &open, flags)) {
        ImGui::End();
        return;
    }

    auto* rootDraw = ImGui::GetWindowDrawList();
    appearance.DrawBackground(rootDraw, ImGui::GetWindowPos(),
        {ImGui::GetWindowPos().x + ImGui::GetWindowSize().x, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y});

    DrawBanner();
    ImGui::Dummy(ImVec2(0.0F, 5.0F));

    constexpr float navigationWidth = 215.0F;
    constexpr float detailsWidth = 260.0F;
    constexpr float footerHeight = 52.0F;
    constexpr float spacing = 8.0F;
    const float panelAlpha = appearance.BackgroundEnabled() ? 0.78F : 0.985F;

    const ImVec2 bodyAvailable = ImGui::GetContentRegionAvail();
    const float bodyHeight = (std::max)(300.0F, bodyAvailable.y - footerHeight - spacing);
    const float centerWidth = (std::max)(420.0F, bodyAvailable.x - navigationWidth - detailsWidth - spacing * 2.0F);

    const ImVec2 navMin = ImGui::GetCursorScreenPos();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4{0.030F, 0.022F, 0.020F, panelAlpha});
    if (ImGui::BeginChild("##DevilsDenNavigation", ImVec2(navigationWidth, bodyHeight), ImGuiChildFlags_Borders))
        DrawNavigation();
    ImGui::EndChild();
    ImGui::PopStyleColor();
    theme.DrawOrnateFrame(rootDraw, navMin, ImVec2{navMin.x + navigationWidth, navMin.y + bodyHeight}, false);

    ImGui::SameLine(0.0F, spacing);
    const ImVec2 contentMin = ImGui::GetCursorScreenPos();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4{0.032F, 0.023F, 0.020F, panelAlpha});
    if (ImGui::BeginChild("##DevilsDenContent", ImVec2(centerWidth, bodyHeight), ImGuiChildFlags_Borders)) {
        if (Supplemental::SelectedPage == Supplemental::Page::Misc) {
            Misc::DrawMiscPage();
        }
        else if (Supplemental::SelectedPage == Supplemental::Page::Recovery) {
            Recovery::DrawRecoveryPage();
        }
        else if (Supplemental::SelectedPage == Supplemental::Page::Debug) {
            Debug::DrawDebugPage();
        }
        else {
            switch (m_page) {
            case Page::Self: Self::DrawSelfPage(); break;
            case Page::Weapons: DrawWeaponsPage(); break;
            case Page::Vehicle: DrawVehiclePage(); break;
            case Page::Teleport: DrawTeleportPage(); break;
            case Page::World: DrawPlaceholderPage("WORLD", "World and environment controls will live here."); break;
            case Page::Network: Network::DrawNetworkPage(); break;
            case Page::Defense: Defense::DrawDefensePage(); break;
            case Page::Lua: Lua::DrawLuaPage(); break;
            case Page::Settings: DrawSettingsPage(); break;
            }
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    theme.DrawOrnateFrame(rootDraw, contentMin, ImVec2{contentMin.x + centerWidth, contentMin.y + bodyHeight}, false);

    ImGui::SameLine(0.0F, spacing);
    const ImVec2 detailsMin = ImGui::GetCursorScreenPos();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4{0.028F, 0.020F, 0.018F, panelAlpha});
    if (ImGui::BeginChild("##DevilsDenDetails", ImVec2(detailsWidth, bodyHeight), ImGuiChildFlags_Borders)) {
        const char* detailTitle = "PLAYER";
        const char* detailText = "Modify player abilities, protection and movement through the live Story Mode runtime.";
        const char* detailTag = "SELF / PLAYER";
        Themes::Menu_Icon detailIcon = Themes::Menu_Icon::Self;

        if (Supplemental::SelectedPage == Supplemental::Page::Misc) {
            detailTitle = "MISC";
            detailText = "Business tools, business status views and related utilities.";
            detailTag = "MISC / BUSINESS";
            detailIcon = Themes::Menu_Icon::World;
        }
        else if (Supplemental::SelectedPage == Supplemental::Page::Recovery) {
            detailTitle = "RECOVERY";
            detailText = "Progression, unlock and collection status tools.";
            detailTag = "RECOVERY / PROGRESSION";
            detailIcon = Themes::Menu_Icon::Settings;
        }
        else if (Supplemental::SelectedPage == Supplemental::Page::Debug) {
            detailTitle = "DEBUG";
            detailText = "Raw regular and packed stat inspection, editing and batch import tools.";
            detailTag = "DEBUG / STAT EDITOR";
            detailIcon = Themes::Menu_Icon::Settings;
        }
        else {
            switch (m_page) {
            case Page::Self:
                detailTitle = "PLAYER";
                detailText = "Player protection, wanted state, movement and live character options.";
                detailTag = "SELF / PLAYER";
                detailIcon = Themes::Menu_Icon::Self;
                break;
            case Page::Weapons:
                detailTitle = "ARMORY";
                detailText = "Weapons, ammunition, loadout actions and live bullet effects.";
                detailTag = "WEAPONS / COMBAT";
                detailIcon = Themes::Menu_Icon::Weapons;
                break;
            case Page::Vehicle:
                detailTitle = "DEVILS FORGE";
                detailText = "Spawn, customize, preserve and rebuild GTA V Enhanced vehicles.";
                detailTag = "VEHICLE / FORGE";
                detailIcon = Themes::Menu_Icon::Vehicle;
                break;
            case Page::Teleport:
                detailTitle = "WAYFINDER";
                detailText = "Waypoint and preset travel tools for the player or occupied vehicle.";
                detailTag = "TELEPORT / MAP";
                detailIcon = Themes::Menu_Icon::Teleport;
                break;
            case Page::World:
                detailTitle = "WORLD";
                detailText = "World and environment controls will live in this domain as they are added.";
                detailTag = "WORLD / ENVIRONMENT";
                detailIcon = Themes::Menu_Icon::World;
                break;
            case Page::Network:
                detailTitle = "NETWORK";
                detailText = "Sessions, players and related network/session functionality are organized in the Network domain.";
                detailTag = "NETWORK / SESSIONS";
                detailIcon = Themes::Menu_Icon::Network;
                break;
            case Page::Defense:
                detailTitle = "DEFENSE";
                detailText = "Local safety policies, defensive runtime status and activity monitoring.";
                detailTag = "DEFENSE / PLAYER SAFETY";
                detailIcon = Themes::Menu_Icon::Network;
                break;
            case Page::Lua:
                detailTitle = "LUA SCRIPTING";
                detailText = "Embedded Lua 5.4 runtime powered by Sol2, with protected execution ready for future bindings.";
                detailTag = "LUA / SCRIPTS";
                detailIcon = Themes::Menu_Icon::Lua;
                break;
            case Page::Settings:
                detailTitle = "SETTINGS";
                detailText = "Themes, images, icons, fonts, style and complete configuration live here.";
                detailTag = "SETTINGS / APPEARANCE";
                detailIcon = Themes::Menu_Icon::Settings;
                break;
            }
        }

        ImGui::TextColored(palette.emberRed, "OPTION DETAILS");
        MedievalDivider();
        const ImVec2 iconOrigin = ImGui::GetCursorScreenPos();
        const float iconCenterX = iconOrigin.x + ImGui::GetContentRegionAvail().x * 0.5F;
        const ImVec2 iconCenter{iconCenterX, iconOrigin.y + 43.0F};
        auto* detailDraw = ImGui::GetWindowDrawList();
        detailDraw->AddCircleFilled(iconCenter, 35.0F, ImGui::GetColorU32(ImVec4{0.075F, 0.020F, 0.017F, 0.82F}), 30);
        detailDraw->AddCircle(iconCenter, 36.0F, ImGui::GetColorU32(palette.emberGlow), 30, 3.0F);
        detailDraw->AddCircle(iconCenter, 30.0F, ImGui::GetColorU32(palette.bronzeDark), 30, 1.0F);
        if (!appearance.DrawCustomIcon(detailIcon, detailDraw, iconCenter, 22.0F))
            theme.DrawIcon(detailIcon, detailDraw, iconCenter, 20.0F, ImGui::GetColorU32(palette.bronze));
        ImGui::Dummy(ImVec2(0.0F, 88.0F));

        const float titleWidth = ImGui::CalcTextSize(detailTitle).x;
        ImGui::SetCursorPosX((std::max)(ImGui::GetStyle().WindowPadding.x,
            (ImGui::GetWindowWidth() - titleWidth) * 0.5F));
        ImGui::TextColored(palette.emberRed, "%s", detailTitle);
        ImGui::Spacing();
        ImGui::TextWrapped("%s", detailText);
        MedievalDivider();
        ImGui::TextColored(palette.bronze, "DOMAIN");
        ImGui::TextWrapped("%s", detailTag);
        MedievalDivider();
        ImGui::TextColored(palette.bronze, "RUNTIME");
        ImGui::TextColored(ImVec4{0.42F, 0.78F, 0.42F, 1.0F}, "LIVE APPLY");
        ImGui::TextWrapped("Appearance changes are previewed and applied live; persistent settings restore on startup.");

        ImGui::SetCursorPosY((std::max)(ImGui::GetCursorPosY(), ImGui::GetWindowHeight() - 74.0F));
        ImGui::TextDisabled("INSERT");
        ImGui::SameLine();
        ImGui::TextColored(palette.bronze, "toggle menu");
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    theme.DrawOrnateFrame(rootDraw, detailsMin, ImVec2{detailsMin.x + detailsWidth, detailsMin.y + bodyHeight}, false);

    ImGui::Dummy(ImVec2(0.0F, spacing));
    const ImVec2 footerMin = ImGui::GetCursorScreenPos();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4{0.030F, 0.020F, 0.018F, appearance.BackgroundEnabled() ? 0.82F : 0.995F});
    if (ImGui::BeginChild("##DevilsDenFooter", ImVec2(0.0F, footerHeight), ImGuiChildFlags_Borders)) {
        if (ImGui::Button("CONFIGS", ImVec2(160.0F, 32.0F))) {
            Supplemental::SelectedPage = Supplemental::Page::None;
            m_page = Page::Settings;
        }
        ImGui::SameLine();
        if (ImGui::Button("PLAYER", ImVec2(160.0F, 32.0F))) {
            Supplemental::SelectedPage = Supplemental::Page::None;
            m_page = Page::Self;
        }
        ImGui::SameLine();
        if (ImGui::Button("DEVILS FORGE", ImVec2(180.0F, 32.0F))) {
            Supplemental::SelectedPage = Supplemental::Page::None;
            m_page = Page::Vehicle;
        }
        ImGui::SameLine();
        ImGui::TextColored(palette.bronze, "LIVE CHANGES");
        ImGui::SameLine();
        ImGui::TextDisabled("apply immediately");
        const float closeWidth = 150.0F;
        ImGui::SameLine((std::max)(ImGui::GetCursorPosX() + 8.0F, ImGui::GetWindowWidth() - closeWidth - 12.0F));
        if (ImGui::Button("CLOSE MENU", ImVec2(closeWidth, 32.0F))) open = false;
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    theme.DrawOrnateFrame(rootDraw, footerMin,
        ImVec2{footerMin.x + ImGui::GetContentRegionAvail().x, footerMin.y + footerHeight}, true);

    ImGui::End();
}
}
