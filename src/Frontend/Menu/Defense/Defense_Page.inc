#include "Defense_Page.hpp"

#include "Frontend/Menu/Themes/Menu_Theme.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Gameplay_State.hpp"

#include <imgui.h>

namespace Devilz::Frontend::Defense
{
namespace
{
using Devilz::Integrations::GTA5_Enhanced::GTA_Gameplay_State;

void DrawSectionDivider()
{
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}

void DrawPendingOption(const char* label, const char* description)
{
    bool enabled = false;
    ImGui::BeginDisabled(true);
    ImGui::Checkbox(label, &enabled);
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::TextDisabled("[VERIFYING ENHANCED PATH]");
    ImGui::Indent();
    ImGui::TextWrapped("%s", description);
    ImGui::Unindent();
    ImGui::Spacing();
}

void DrawLiveToggle(
    const char* label,
    const char* description,
    bool enabled,
    void (*setter)(GTA_Gameplay_State&, bool))
{
    auto& state = GTA_Gameplay_State::Instance();
    bool value = enabled;
    if (ImGui::Checkbox(label, &value)) {
        setter(state, value);
    }

    ImGui::Indent();
    ImGui::TextDisabled("%s", description);
    ImGui::Unindent();
    ImGui::Spacing();
}

void SetGodMode(GTA_Gameplay_State& state, bool enabled) { state.SetGodMode(enabled); }
void SetNoRagdoll(GTA_Gameplay_State& state, bool enabled) { state.SetNoRagdoll(enabled); }
void SetKeepPlayerClean(GTA_Gameplay_State& state, bool enabled) { state.SetKeepPlayerClean(enabled); }
void SetNeverWanted(GTA_Gameplay_State& state, bool enabled) { state.SetNeverWanted(enabled); }

void DrawPlayerProtection()
{
    auto& theme = Themes::Menu_Theme_Manager::Instance();
    const auto& palette = theme.Palette();
    auto& state = GTA_Gameplay_State::Instance();

    ImGui::TextColored(palette.bronze, "PLAYER PROTECTION");
    DrawSectionDivider();

    DrawLiveToggle(
        "Damage Protection",
        "Keeps the local player protected using Devilz_Den's existing God Mode runtime path.",
        state.GodMode(),
        SetGodMode);

    DrawLiveToggle(
        "Ragdoll Protection",
        "Prevents disruptive ragdoll state while enabled.",
        state.NoRagdoll(),
        SetNoRagdoll);

    DrawLiveToggle(
        "Wanted Level Guard",
        "Continuously clears wanted-state pressure through the existing gameplay runner.",
        state.NeverWanted(),
        SetNeverWanted);

    DrawLiveToggle(
        "Keep Player Clean",
        "Continuously clears visible damage, blood and dirt from the local player.",
        state.KeepPlayerClean(),
        SetKeepPlayerClean);

    DrawPendingOption(
        "Forced Task Guard",
        "Planned protection against remote/scripted attempts to force unwanted local-player tasks.");

    DrawPendingOption(
        "Position / Teleport Guard",
        "Planned validation for unexpected remote position changes before corrective action is taken.");
}

void DrawNetworkProtection()
{
    auto& theme = Themes::Menu_Theme_Manager::Instance();
    const auto& palette = theme.Palette();

    ImGui::TextColored(palette.bronze, "NETWORK PROTECTION");
    DrawSectionDivider();

    DrawPendingOption(
        "Network Event Validation",
        "Validate incoming net-game events before they reach local gameplay state.");

    DrawPendingOption(
        "Kick / Removal Guard",
        "Detect and reject verified malicious removal/kick event paths that target the local player.");

    DrawPendingOption(
        "Control Request Guard",
        "Validate suspicious network-control requests involving the local player or controlled vehicle.");

    DrawPendingOption(
        "Vehicle Removal Guard",
        "Protect the occupied/personal vehicle from verified malicious removal or invalid state events.");

    DrawPendingOption(
        "Entity Spam Guard",
        "Rate-limit or reject verified abusive entity/network-event patterns without scanning every frame.");
}

void DrawScriptProtection()
{
    auto& theme = Themes::Menu_Theme_Manager::Instance();
    const auto& palette = theme.Palette();

    ImGui::TextColored(palette.bronze, "SCRIPT PROTECTION");
    DrawSectionDivider();

    DrawPendingOption(
        "Script Event Validation",
        "Validate incoming scripted-game-event IDs, sender, target mask and argument structure.");

    DrawPendingOption(
        "Script Kick Guard",
        "Reject verified scripted removal/kick paths instead of blindly blocking legitimate session traffic.");

    DrawPendingOption(
        "Forced Activity Guard",
        "Protect against verified scripted events that force unwanted activities, transitions or states.");

    DrawPendingOption(
        "Camera / Cutscene Guard",
        "Reject verified remote/scripted camera or cutscene disturbance paths.");

    DrawPendingOption(
        "Malformed Payload Guard",
        "Fail closed on verified malformed scripted-event payloads and impossible argument layouts.");
}

void DrawDisturbanceProtection()
{
    auto& theme = Themes::Menu_Theme_Manager::Instance();
    const auto& palette = theme.Palette();

    ImGui::TextColored(palette.bronze, "DISTURBANCE PROTECTION");
    DrawSectionDivider();

    DrawPendingOption(
        "Sound Spam Guard",
        "Detect and suppress verified abusive network/script sound-event spam directed at the local client.");

    DrawPendingOption(
        "Explosion Spam Guard",
        "Rate-limit verified abusive explosion-event patterns while allowing legitimate world activity.");

    DrawPendingOption(
        "Camera Shake Guard",
        "Protect against verified externally-triggered camera disturbance without disabling local effects globally.");

    DrawPendingOption(
        "Particle / Effect Spam Guard",
        "Detect verified abusive synchronized effect patterns before they can flood the local client.");
}

void DrawActivityRow(const char* source, const char* category, const char* status, int count)
{
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(source);
    ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(category);
    ImGui::TableSetColumnIndex(2); ImGui::TextUnformatted(status);
    ImGui::TableSetColumnIndex(3); ImGui::Text("%d", count);
}

void DrawActivity()
{
    auto& theme = Themes::Menu_Theme_Manager::Instance();
    const auto& palette = theme.Palette();
    auto& state = GTA_Gameplay_State::Instance();

    const int activeCount =
        (state.GodMode() ? 1 : 0) +
        (state.NoRagdoll() ? 1 : 0) +
        (state.NeverWanted() ? 1 : 0) +
        (state.KeepPlayerClean() ? 1 : 0);

    ImGui::TextColored(palette.bronze, "DEFENSE ACTIVITY");
    DrawSectionDivider();

    ImGui::TextDisabled("Active local guards");
    ImGui::SameLine(190.0F);
    ImGui::TextColored(palette.parchment, "%d", activeCount);

    ImGui::TextDisabled("Event interception");
    ImGui::SameLine(190.0F);
    ImGui::TextDisabled("Pending verified hooks");

    DrawSectionDivider();

    constexpr ImGuiTableFlags flags =
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_SizingStretchProp;

    if (ImGui::BeginTable("##DefenseActivityTable", 4, flags)) {
        ImGui::TableSetupColumn("SOURCE");
        ImGui::TableSetupColumn("CATEGORY");
        ImGui::TableSetupColumn("STATUS");
        ImGui::TableSetupColumn("COUNT");
        ImGui::TableHeadersRow();

        DrawActivityRow("Local", "Damage", state.GodMode() ? "Protected" : "Off", state.GodMode() ? 1 : 0);
        DrawActivityRow("Local", "Ragdoll", state.NoRagdoll() ? "Protected" : "Off", state.NoRagdoll() ? 1 : 0);
        DrawActivityRow("Local", "Wanted", state.NeverWanted() ? "Protected" : "Off", state.NeverWanted() ? 1 : 0);
        DrawActivityRow("Local", "Clean state", state.KeepPlayerClean() ? "Protected" : "Off", state.KeepPlayerClean() ? 1 : 0);
        DrawActivityRow("Network", "Inbound events", "Verification pending", 0);
        DrawActivityRow("Script", "Inbound events", "Verification pending", 0);

        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::TextDisabled(
        "Blocked-event counters will activate only after the Enhanced receive hooks and event layouts are verified.");
}
}

void DrawDefensePage()
{
    auto& theme = Themes::Menu_Theme_Manager::Instance();
    const auto& palette = theme.Palette();

    ImGui::TextColored(palette.emberRed, "DEFENSE");
    ImGui::SameLine();
    ImGui::TextDisabled("- local safety, event validation and disturbance protection");

    DrawSectionDivider();

    ImGui::TextColored(palette.bronze, "DEFENSE STATUS");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4{0.30F, 0.85F, 0.40F, 1.0F}, "[LOCAL GUARDS ONLINE]");

    ImGui::Spacing();
    ImGui::TextWrapped(
        "Local protection uses existing Devilz_Den runtime features now. Network/script protections are exposed as "
        "planned guards but remain locked until their GTA V Enhanced receive paths, payloads and signatures are verified.");

    ImGui::Spacing();

    if (ImGui::BeginTabBar("##DefensePageTabs")) {
        if (ImGui::BeginTabItem("PLAYER")) {
            DrawPlayerProtection();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("NETWORK")) {
            DrawNetworkProtection();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("SCRIPT")) {
            DrawScriptProtection();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("DISTURBANCE")) {
            DrawDisturbanceProtection();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("ACTIVITY")) {
            DrawActivity();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}
}
