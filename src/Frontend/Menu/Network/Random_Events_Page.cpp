#include "Random_Events_Page.hpp"

#include "Frontend/Menu/Themes/Menu_Theme.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Random_Events_Extension.hpp"

#include <imgui.h>

#include <algorithm>
#include <cstdio>
#include <limits>
#include <string>

namespace Devilz::Frontend::Network
{
namespace
{
using Integrations::GTA5_Enhanced::GTA_Random_Event_Action_Status;
using Integrations::GTA5_Enhanced::GTA_Random_Event_Id;
using Integrations::GTA5_Enhanced::GTA_Random_Event_Info;
using Integrations::GTA5_Enhanced::GTA_Random_Event_State;
using Integrations::GTA5_Enhanced::GTA_Random_Event_Count;
using Integrations::GTA5_Enhanced::GTA_Random_Event_Name;
using Integrations::GTA5_Enhanced::GetRandomEventsSnapshot;
using Integrations::GTA5_Enhanced::RequestKillRandomEvent;
using Integrations::GTA5_Enhanced::RequestLaunchRandomEvent;
using Integrations::GTA5_Enhanced::RequestSetRandomEventAvailability;
using Integrations::GTA5_Enhanced::RequestSetRandomEventCooldown;
using Integrations::GTA5_Enhanced::RequestTeleportToRandomEvent;
using Integrations::GTA5_Enhanced::SelectRandomEvent;

const char* StateText(GTA_Random_Event_State state) noexcept
{
    switch (state) {
    case GTA_Random_Event_State::Inactive: return "Inactive";
    case GTA_Random_Event_State::Available: return "Available";
    case GTA_Random_Event_State::Active: return "Active";
    case GTA_Random_Event_State::Cleanup: return "Cleanup";
    default: return "N/A";
    }
}

const char* ActionStatusText(GTA_Random_Event_Action_Status status) noexcept
{
    switch (status) {
    case GTA_Random_Event_Action_Status::Unavailable: return "Unavailable";
    case GTA_Random_Event_Action_Status::Ready: return "Ready";
    case GTA_Random_Event_Action_Status::Queued: return "Queued";
    case GTA_Random_Event_Action_Status::RequestSent: return "Request sent";
    case GTA_Random_Event_Action_Status::Succeeded: return "Succeeded";
    case GTA_Random_Event_Action_Status::AlreadyActive: return "Already active";
    case GTA_Random_Event_Action_Status::NotActive: return "Not active";
    case GTA_Random_Event_Action_Status::CoordinatesUnavailable: return "Coordinates unavailable";
    case GTA_Random_Event_Action_Status::Failed: return "Failed";
    default: return "Unknown";
    }
}

bool ActionBusy(GTA_Random_Event_Action_Status status) noexcept
{
    return status == GTA_Random_Event_Action_Status::Queued;
}

bool ActionSucceeded(GTA_Random_Event_Action_Status status) noexcept
{
    return status == GTA_Random_Event_Action_Status::Ready ||
           status == GTA_Random_Event_Action_Status::RequestSent ||
           status == GTA_Random_Event_Action_Status::Succeeded;
}

ImVec4 EventStateColor(
    GTA_Random_Event_State state,
    const Themes::Menu_Theme_Palette& palette) noexcept
{
    switch (state) {
    case GTA_Random_Event_State::Active: return ImVec4{0.24F, 0.88F, 0.30F, 1.0F};
    case GTA_Random_Event_State::Available: return palette.parchment;
    case GTA_Random_Event_State::Inactive:
    case GTA_Random_Event_State::Cleanup: return palette.emberRed;
    default: return palette.disabledText;
    }
}

std::string FormatDuration(int milliseconds)
{
    const int totalSeconds = (std::max)(0, milliseconds) / 1000;
    const int hours = totalSeconds / 3600;
    const int minutes = (totalSeconds % 3600) / 60;
    const int seconds = totalSeconds % 60;

    char buffer[24]{};
    if (hours > 0)
        std::snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", hours, minutes, seconds);
    else
        std::snprintf(buffer, sizeof(buffer), "%02d:%02d", minutes, seconds);
    return buffer;
}

int ToMilliseconds(int value, bool minutes) noexcept
{
    value = (std::max)(0, value);
    if (!minutes)
        return value;

    constexpr int maximumMinutes = (std::numeric_limits<int>::max)() / 60000;
    return (std::min)(value, maximumMinutes) * 60000;
}

void SyncTimeInputs(
    const GTA_Random_Event_Info& event,
    bool minutes,
    int& cooldown,
    int& availability) noexcept
{
    if (minutes) {
        cooldown = event.inactiveTimeMs / 60000;
        availability = event.availableTimeMs / 60000;
    } else {
        cooldown = event.inactiveTimeMs;
        availability = event.availableTimeMs;
    }
}
}

void DrawRandomEventsPanel()
{
    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();
    const auto snapshot = GetRandomEventsSnapshot();

    static int selectedIndex = 0;
    static int selectedSubvariation = 0;
    static int cooldown = 1800000;
    static int availability = 900000;
    static bool applyInMinutes = false;
    static bool syncTimeInputs = true;

    selectedIndex = std::clamp(
        selectedIndex,
        0,
        static_cast<int>(GTA_Random_Event_Count) - 1);
    auto selectedEvent = static_cast<GTA_Random_Event_Id>(selectedIndex);
    SelectRandomEvent(selectedEvent);

    Themes::Menu_Theme_Manager::Instance().DrawDivider();
    ImGui::TextColored(palette.bronze, "RANDOM EVENTS");
    ImGui::SameLine();
    if (snapshot.runtimeReady && snapshot.freemodeRunning && snapshot.clientInitialized)
        ImGui::TextDisabled("[%s]", ActionStatusText(snapshot.actionStatus));
    else
        ImGui::TextColored(palette.emberRed, "[Unavailable]");

    ImGui::TextWrapped(
        "Launch and manage freemode random events through the verified Enhanced script runtime. "
        "Launch and timing changes require freemode script host authority.");

    if (!snapshot.runtimeReady || !snapshot.freemodeRunning || !snapshot.clientInitialized)
        ImGui::TextColored(palette.emberRed, "%s", snapshot.runtimeDetail.c_str());

    if (ImGui::BeginCombo("Select Event", GTA_Random_Event_Name(selectedEvent))) {
        for (std::size_t index = 0; index < GTA_Random_Event_Count; ++index) {
            const auto candidate = static_cast<GTA_Random_Event_Id>(index);
            const bool selected = selectedIndex == static_cast<int>(index);
            ImGui::PushStyleColor(
                ImGuiCol_Text,
                EventStateColor(snapshot.events[index].state, palette));
            if (ImGui::Selectable(GTA_Random_Event_Name(candidate), selected)) {
                selectedIndex = static_cast<int>(index);
                selectedSubvariation = 0;
                syncTimeInputs = true;
                SelectRandomEvent(candidate);
            }
            ImGui::PopStyleColor();
            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    selectedEvent = static_cast<GTA_Random_Event_Id>(selectedIndex);
    SelectRandomEvent(selectedEvent);
    const auto& event = snapshot.events[static_cast<std::size_t>(selectedIndex)];
    if (syncTimeInputs && snapshot.freemodeRunning && snapshot.clientInitialized) {
        SyncTimeInputs(event, applyInMinutes, cooldown, availability);
        syncTimeInputs = false;
    }

    const int maxSubvariation = std::clamp(event.maxSubvariation, 0, 255);
    char locationLabel[64]{};
    std::snprintf(
        locationLabel,
        sizeof(locationLabel),
        "Select Location (0-%d)",
        maxSubvariation);
    if (ImGui::InputInt(locationLabel, &selectedSubvariation))
        selectedSubvariation = std::clamp(selectedSubvariation, 0, maxSubvariation);

    ImGui::Text("Locally Active Events: %d", snapshot.activeEventCount);

    const bool disabled = !snapshot.runtimeReady || !snapshot.freemodeRunning ||
                          !snapshot.clientInitialized || ActionBusy(snapshot.actionStatus);
    ImGui::BeginDisabled(disabled);

    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float availableWidth = ImGui::GetContentRegionAvail().x;
    const float actionWidth = (std::max)(90.0F, (availableWidth - (spacing * 2.0F)) / 3.0F);

    if (ImGui::Button("Launch Event", ImVec2{actionWidth, 0.0F}))
        (void)RequestLaunchRandomEvent(selectedEvent, selectedSubvariation);
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip("Requires freemode script host authority.");

    ImGui::SameLine();
    if (ImGui::Button("Kill Event", ImVec2{actionWidth, 0.0F}))
        (void)RequestKillRandomEvent(selectedEvent);

    ImGui::SameLine();
    if (ImGui::Button("Teleport", ImVec2{actionWidth, 0.0F}))
        (void)RequestTeleportToRandomEvent(selectedEvent);

    ImGui::EndDisabled();

    ImGui::Text("State: %s", StateText(event.state));
    if (event.state == GTA_Random_Event_State::Inactive) {
        ImGui::SameLine();
        ImGui::TextDisabled("- launching in %s", FormatDuration(event.remainingTimeMs).c_str());
        ImGui::Text("Location: N/A");
        ImGui::Text("Trigger Range: N/A");
    } else if (event.state == GTA_Random_Event_State::Unknown) {
        ImGui::Text("Location: N/A");
        ImGui::Text("Trigger Range: N/A");
    } else {
        if (event.state == GTA_Random_Event_State::Available) {
            ImGui::SameLine();
            ImGui::TextDisabled("- deactivating in %s", FormatDuration(event.remainingTimeMs).c_str());
        }
        ImGui::Text("Location: %d", event.subvariation);
        ImGui::Text("Trigger Range: %.2f", event.triggerRange);
    }

    if (event.state == GTA_Random_Event_State::Active)
        ImGui::TextDisabled("Event script: %s", event.scriptRunning ? "running" : "not found locally");

    if (!snapshot.actionDetail.empty()) {
        const ImVec4 statusColor = ActionSucceeded(snapshot.actionStatus)
            ? palette.bronze
            : palette.emberRed;
        ImGui::TextColored(statusColor, "%s", snapshot.actionDetail.c_str());
    }

    ImGui::SeparatorText("Cooldown & Availability");

    const char* unit = applyInMinutes ? "minutes" : "ms";
    const float setButtonWidth = ImGui::CalcTextSize("Set Availability").x +
                                 (ImGui::GetStyle().FramePadding.x * 2.0F);
    const float timeInputWidth = (std::max)(
        80.0F,
        ImGui::GetContentRegionAvail().x - setButtonWidth - spacing);

    ImGui::Text("Cooldown (%s)", unit);
    ImGui::SetNextItemWidth(timeInputWidth);
    if (ImGui::InputInt("##random_event_cooldown", &cooldown))
        cooldown = (std::max)(0, cooldown);
    ImGui::SameLine();
    ImGui::BeginDisabled(disabled);
    if (ImGui::Button("Set Cooldown"))
        (void)RequestSetRandomEventCooldown(
            selectedEvent,
            ToMilliseconds(cooldown, applyInMinutes));
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip("Requires freemode script host authority.");

    ImGui::Text("Availability (%s)", unit);
    ImGui::SetNextItemWidth(timeInputWidth);
    if (ImGui::InputInt("##random_event_availability", &availability))
        availability = (std::max)(0, availability);
    ImGui::SameLine();
    ImGui::BeginDisabled(disabled);
    if (ImGui::Button("Set Availability"))
        (void)RequestSetRandomEventAvailability(
            selectedEvent,
            ToMilliseconds(availability, applyInMinutes));
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip("Requires freemode script host authority.");

    const bool wasMinutes = applyInMinutes;
    if (ImGui::Checkbox("Apply in Minutes", &applyInMinutes) && wasMinutes != applyInMinutes) {
        if (applyInMinutes) {
            cooldown /= 60000;
            availability /= 60000;
        } else {
            cooldown = ToMilliseconds(cooldown, true);
            availability = ToMilliseconds(availability, true);
        }
    }
}
}
