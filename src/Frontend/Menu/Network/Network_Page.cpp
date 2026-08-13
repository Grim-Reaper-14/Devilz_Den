#include "Network_Page.hpp"

#include "Random_Events_Page.hpp"

#include "Frontend/Menu/Themes/Menu_Theme.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Network_Session_State.hpp"

#include <imgui.h>

#include <algorithm>

namespace Devilz::Frontend::Network
{
namespace
{
using Integrations::GTA5_Enhanced::GTA_Network_Join_Type;
using Integrations::GTA5_Enhanced::GTA_Network_Session_State;
using Integrations::GTA5_Enhanced::GTA_Network_Session_Status;

const char* SessionStatusText(GTA_Network_Session_Status status) noexcept
{
    switch (status) {
    case GTA_Network_Session_Status::Unavailable: return "Backend unavailable";
    case GTA_Network_Session_Status::Ready: return "Ready";
    case GTA_Network_Session_Status::Queued: return "Queued";
    case GTA_Network_Session_Status::Switching: return "Switching session...";
    case GTA_Network_Session_Status::Success: return "Transition requested";
    case GTA_Network_Session_Status::Failed: return "Transition failed - check Devilz_Den log";
    default: return "Unknown";
    }
}

bool SessionBusy(GTA_Network_Session_Status status) noexcept
{
    return status == GTA_Network_Session_Status::Queued ||
           status == GTA_Network_Session_Status::Switching;
}

void SessionButton(const char* label, GTA_Network_Join_Type type, float width)
{
    if (ImGui::Button(label, ImVec2{width, 0.0F}))
        (void)GTA_Network_Session_State::Instance().Request(type);
}
}

void DrawNetworkPage()
{
    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();
    auto& sessions = GTA_Network_Session_State::Instance();
    const auto status = sessions.Status();

    ImGui::TextColored(palette.emberRed, "NETWORK");
    ImGui::SameLine();
    ImGui::TextDisabled("- sessions, random events and players");
    Themes::Menu_Theme_Manager::Instance().DrawDivider();

    ImGui::TextColored(palette.bronze, "SESSIONS");
    ImGui::SameLine();
    if (sessions.RuntimeReady())
        ImGui::TextDisabled("[%s]", SessionStatusText(status));
    else
        ImGui::TextColored(palette.emberRed, "[%s]", SessionStatusText(status));

    ImGui::TextWrapped(
        "Switch GTA Online session type through the Enhanced shop_controller transition path. "
        "Requests are queued onto the GTA script thread instead of running from the ImGui render thread.");

    const bool disabled = !sessions.RuntimeReady() || SessionBusy(status);
    ImGui::BeginDisabled(disabled);

    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float available = ImGui::GetContentRegionAvail().x;
    const float buttonWidth = (std::max)(120.0F, (available - spacing) * 0.5F);

    SessionButton("Join Public", GTA_Network_Join_Type::JoinPublic, buttonWidth);
    ImGui::SameLine();
    SessionButton("New Public", GTA_Network_Join_Type::NewPublic, buttonWidth);

    SessionButton("Invite Only", GTA_Network_Join_Type::InviteOnly, buttonWidth);
    ImGui::SameLine();
    SessionButton("Solo", GTA_Network_Join_Type::Solo, buttonWidth);

    SessionButton("Crew", GTA_Network_Join_Type::Crew, buttonWidth);
    ImGui::SameLine();
    SessionButton("Closed Crew", GTA_Network_Join_Type::ClosedCrew, buttonWidth);

    SessionButton("Join Crew", GTA_Network_Join_Type::JoinCrew, buttonWidth);
    ImGui::SameLine();
    SessionButton("Find Friend", GTA_Network_Join_Type::FindFriend, buttonWidth);

    SessionButton("Closed Friends", GTA_Network_Join_Type::ClosedFriends, buttonWidth);
    ImGui::SameLine();
    SessionButton("SCTV", GTA_Network_Join_Type::ScTv, buttonWidth);

    ImGui::Spacing();
    SessionButton("Leave GTA Online", GTA_Network_Join_Type::LeaveOnline, available);

    ImGui::EndDisabled();

    DrawRandomEventsPanel();

    Themes::Menu_Theme_Manager::Instance().DrawDivider();
    ImGui::TextColored(palette.bronze, "PLAYERS");
    ImGui::TextDisabled("Player list module reserved for the Network/Players domain.");
}
}
