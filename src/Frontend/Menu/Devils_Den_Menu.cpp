#include "Devils_Den_Menu.hpp"

#include "Integrations/GTA5_Enhanced/Runtime/GTA_Gameplay_State.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Teleport_Locations.hpp"

#include <imgui.h>

#include <array>

namespace Devilz::Frontend
{
namespace
{
using Integrations::GTA5_Enhanced::GTA_Gameplay_State;
using Integrations::GTA5_Enhanced::GTA_Teleport_Locations;
using Integrations::GTA5_Enhanced::GTA_Teleport_Waypoint_Status;

constexpr ImVec4 EmberRed{0.88F, 0.10F, 0.045F, 1.00F};
constexpr ImVec4 Bronze{0.78F, 0.66F, 0.44F, 1.00F};
constexpr ImVec4 Iron{0.13F, 0.11F, 0.10F, 1.00F};
constexpr ImVec4 DeepStone{0.055F, 0.045F, 0.040F, 1.00F};

const char* TeleportStatusText(GTA_Teleport_Waypoint_Status status) noexcept
{
    switch (status) {
    case GTA_Teleport_Waypoint_Status::Idle: return "Ready";
    case GTA_Teleport_Waypoint_Status::Queued: return "Queued";
    case GTA_Teleport_Waypoint_Status::Resolving: return "Resolving terrain...";
    case GTA_Teleport_Waypoint_Status::Succeeded: return "Teleport complete";
    case GTA_Teleport_Waypoint_Status::NoWaypoint: return "No waypoint is active";
    case GTA_Teleport_Waypoint_Status::Failed: return "Teleport failed - see runtime log";
    default: return "Unknown";
    }
}

void MedievalDivider()
{
    const auto start = ImGui::GetCursorScreenPos();
    const auto width = ImGui::GetContentRegionAvail().x;
    auto* draw = ImGui::GetWindowDrawList();
    draw->AddLine(
        ImVec2(start.x, start.y + 4.0F),
        ImVec2(start.x + width, start.y + 4.0F),
        ImGui::GetColorU32(ImVec4(0.42F, 0.08F, 0.05F, 0.95F)),
        2.0F);
    draw->AddCircleFilled(
        ImVec2(start.x + width * 0.5F, start.y + 4.0F),
        4.0F,
        ImGui::GetColorU32(EmberRed));
    ImGui::Dummy(ImVec2(0.0F, 11.0F));
}
}

void Devils_Den_Menu::Draw(bool& open)
{
    if (!open)
        return;

    ImGui::SetNextWindowSize(ImVec2(920.0F, 620.0F), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.985F);

    constexpr ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse;

    if (!ImGui::Begin("##DevilsDenRoot", &open, flags)) {
        ImGui::End();
        return;
    }

    DrawBanner();
    MedievalDivider();

    const float navWidth = 180.0F;
    if (ImGui::BeginChild("##DevilsDenNavigation", ImVec2(navWidth, 0.0F), ImGuiChildFlags_Borders))
        DrawNavigation();
    ImGui::EndChild();

    ImGui::SameLine();

    if (ImGui::BeginChild("##DevilsDenContent", ImVec2(0.0F, 0.0F), ImGuiChildFlags_Borders)) {
        switch (m_page) {
        case Page::Self: DrawSelfPage(); break;
        case Page::Weapons: DrawPlaceholderPage("WEAPONS", "The armory page will inherit this same medieval frame."); break;
        case Page::Vehicle: DrawPlaceholderPage("VEHICLE", "The stable and vehicle page is queued for the next pass."); break;
        case Page::Teleport: DrawTeleportPage(); break;
        case Page::World: DrawPlaceholderPage("WORLD", "World and environment controls will live here."); break;
        case Page::Settings: DrawPlaceholderPage("SETTINGS", "Theme, hotkeys, configuration, and diagnostics will live here."); break;
        }
    }
    ImGui::EndChild();
    ImGui::End();
}

void Devils_Den_Menu::DrawBanner()
{
    const auto origin = ImGui::GetCursorScreenPos();
    const auto width = ImGui::GetContentRegionAvail().x;
    constexpr float height = 94.0F;
    auto* draw = ImGui::GetWindowDrawList();

    draw->AddRectFilled(origin, ImVec2(origin.x + width, origin.y + height), ImGui::GetColorU32(ImVec4(0.055F, 0.020F, 0.018F, 1.00F)), 2.0F);
    draw->AddRect(origin, ImVec2(origin.x + width, origin.y + height), ImGui::GetColorU32(ImVec4(0.48F, 0.09F, 0.055F, 1.00F)), 2.0F, 0, 2.0F);

    for (int i = 0; i < 5; ++i) {
        const float inset = 7.0F + static_cast<float>(i) * 6.0F;
        draw->AddLine(ImVec2(origin.x + inset, origin.y + 14.0F), ImVec2(origin.x + inset + 24.0F, origin.y + height - 14.0F), ImGui::GetColorU32(ImVec4(0.22F, 0.055F, 0.038F, 0.45F)), 1.0F);
        draw->AddLine(ImVec2(origin.x + width - inset, origin.y + 14.0F), ImVec2(origin.x + width - inset - 24.0F, origin.y + height - 14.0F), ImGui::GetColorU32(ImVec4(0.22F, 0.055F, 0.038F, 0.45F)), 1.0F);
    }

    const char* title = "DEVILS DEN MENU";
    const auto titleSize = ImGui::CalcTextSize(title);
    const auto titlePos = ImVec2(origin.x + (width - titleSize.x) * 0.5F, origin.y + 27.0F);
    draw->AddText(ImVec2(titlePos.x + 2.0F, titlePos.y + 2.0F), ImGui::GetColorU32(ImVec4(0.0F, 0.0F, 0.0F, 0.85F)), title);
    draw->AddText(titlePos, ImGui::GetColorU32(EmberRed), title);

    const char* subtitle = "GTA V ENHANCED  |  RUNTIME READY";
    const auto subtitleSize = ImGui::CalcTextSize(subtitle);
    draw->AddText(ImVec2(origin.x + (width - subtitleSize.x) * 0.5F, origin.y + 56.0F), ImGui::GetColorU32(Bronze), subtitle);

    ImGui::Dummy(ImVec2(width, height));
}

void Devils_Den_Menu::DrawNavigation()
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, DeepStone);
    ImGui::TextColored(Bronze, "CATEGORIES");
    MedievalDivider();

    struct NavigationEntry { const char* label; Page page; };
    constexpr std::array entries{
        NavigationEntry{"SELF", Page::Self},
        NavigationEntry{"WEAPONS", Page::Weapons},
        NavigationEntry{"VEHICLE", Page::Vehicle},
        NavigationEntry{"TELEPORT", Page::Teleport},
        NavigationEntry{"WORLD", Page::World},
        NavigationEntry{"SETTINGS", Page::Settings}
    };

    for (const auto& entry : entries) {
        const bool selected = m_page == entry.page;
        if (selected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.38F, 0.055F, 0.035F, 1.00F));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.52F, 0.075F, 0.045F, 1.00F));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, Iron);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24F, 0.055F, 0.040F, 1.00F));
        }
        if (ImGui::Button(entry.label, ImVec2(-1.0F, 42.0F)))
            m_page = entry.page;
        ImGui::PopStyleColor(2);
    }

    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 48.0F);
    ImGui::TextDisabled("INSERT  -  toggle menu");
    ImGui::PopStyleColor();
}

void Devils_Den_Menu::DrawSelfPage()
{
    auto& gameplay = GTA_Gameplay_State::Instance();

    ImGui::TextColored(EmberRed, "SELF");
    ImGui::SameLine();
    ImGui::TextDisabled("- live Story Mode features");
    MedievalDivider();

    ImGui::TextColored(Bronze, "PLAYER OPTIONS");
    ImGui::Spacing();

    m_godMode = gameplay.GodMode();
    if (ImGui::Checkbox("God Mode", &m_godMode)) gameplay.SetGodMode(m_godMode);

    m_neverWanted = gameplay.NeverWanted();
    if (ImGui::Checkbox("Never Wanted", &m_neverWanted)) gameplay.SetNeverWanted(m_neverWanted);

    m_superJump = gameplay.SuperJump();
    if (ImGui::Checkbox("Super Jump", &m_superJump)) gameplay.SetSuperJump(m_superJump);

    m_infiniteOxygen = gameplay.InfiniteOxygen();
    if (ImGui::Checkbox("Infinite Oxygen", &m_infiniteOxygen)) gameplay.SetInfiniteOxygen(m_infiniteOxygen);

    m_noRagdoll = gameplay.NoRagdoll();
    if (ImGui::Checkbox("No Ragdoll", &m_noRagdoll)) gameplay.SetNoRagdoll(m_noRagdoll);

    m_keepPlayerClean = gameplay.KeepPlayerClean();
    if (ImGui::Checkbox("Keep Player Clean", &m_keepPlayerClean)) gameplay.SetKeepPlayerClean(m_keepPlayerClean);

    m_infiniteAmmo = gameplay.InfiniteAmmo();
    if (ImGui::Checkbox("Infinite Ammo", &m_infiniteAmmo)) gameplay.SetInfiniteAmmo(m_infiniteAmmo);

    ImGui::BeginDisabled();
    ImGui::Checkbox("Fast Run", &m_fastRun);
    ImGui::SliderFloat("Health", &m_health, 0.0F, 100.0F, "%.0f");
    ImGui::EndDisabled();

    ImGui::Spacing();
    MedievalDivider();
    ImGui::TextColored(Bronze, "WEAPONS / ACTIONS");
    ImGui::Spacing();

    if (ImGui::Button("GIVE ALL WEAPONS", ImVec2(190.0F, 38.0F)))
        gameplay.RequestGiveAllWeapons();
    ImGui::SameLine();
    if (ImGui::Button("GIVE MAX AMMO", ImVec2(190.0F, 38.0F)))
        gameplay.RequestGiveMaxAmmo();

    ImGui::Spacing();
    ImGui::TextDisabled("Live Self features execute only from the validated RunScriptThreads game-thread context.");
}

void Devils_Den_Menu::DrawTeleportPage()
{
    auto& gameplay = GTA_Gameplay_State::Instance();

    ImGui::TextColored(EmberRed, "TELEPORT");
    ImGui::SameLine();
    ImGui::TextDisabled("- waypoint and landmark travel");
    MedievalDivider();

    ImGui::TextColored(Bronze, "WAYPOINT");
    ImGui::Spacing();
    ImGui::TextWrapped("Place a waypoint on the Story Mode map, then use the button below. If you are inside a vehicle, the vehicle teleports with you.");
    ImGui::Spacing();

    const auto status = gameplay.TeleportStatus();
    const bool busy = status == GTA_Teleport_Waypoint_Status::Queued || status == GTA_Teleport_Waypoint_Status::Resolving;

    ImGui::BeginDisabled(busy);
    if (ImGui::Button("TELEPORT TO WAYPOINT", ImVec2(240.0F, 44.0F)))
        gameplay.RequestTeleportToWaypoint();
    ImGui::EndDisabled();

    ImGui::Spacing();
    ImGui::TextColored(Bronze, "STATUS");
    ImGui::SameLine();
    ImGui::TextUnformatted(TeleportStatusText(status));

    ImGui::Spacing();
    MedievalDivider();
    ImGui::TextColored(Bronze, "PLACES");
    ImGui::Spacing();

    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float buttonWidth = (ImGui::GetContentRegionAvail().x - spacing) * 0.5F;

    ImGui::BeginDisabled(busy);
    for (std::size_t index = 0; index < GTA_Teleport_Locations.size(); ++index) {
        const auto& location = GTA_Teleport_Locations[index];
        ImGui::PushID(static_cast<int>(index));
        if (ImGui::Button(location.label.data(), ImVec2(buttonWidth, 34.0F)))
            gameplay.RequestTeleportToLocation(location.id);
        ImGui::PopID();
        if ((index % 2U) == 0U) ImGui::SameLine();
    }
    ImGui::EndDisabled();

    ImGui::Spacing();
    ImGui::TextDisabled("Waypoint travel resolves ground, water, then approximate terrain. Current vehicles are moved with the player.");
}

void Devils_Den_Menu::DrawPlaceholderPage(const char* title, const char* detail)
{
    ImGui::TextColored(EmberRed, "%s", title);
    MedievalDivider();
    ImGui::TextWrapped("%s", detail);
    ImGui::Spacing();
    ImGui::TextDisabled("This page will use the same Devil's Den medieval frame and control language.");
}
}
