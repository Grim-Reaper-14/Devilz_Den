#pragma once

#include "Frontend/Menu/Self/Outfit_Editor_Page.hpp"
#include "Frontend/Menu/Themes/Menu_Theme.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Gameplay_State.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Self_Utility_Extension.hpp"

#include <imgui.h>

namespace Devilz::Frontend::Self
{
inline void DrawSelfPage()
{
    using namespace Integrations::GTA5_Enhanced;

    auto& gameplay = GTA_Gameplay_State::Instance();
    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();

    ImGui::TextColored(palette.emberRed, "SELF");
    ImGui::SameLine();
    ImGui::TextDisabled("- live player and online controls");
    Themes::Menu_Theme_Manager::Instance().DrawDivider();

    if (!ImGui::BeginTabBar("##SelfPageTabs"))
        return;

    if (ImGui::BeginTabItem("PLAYER")) {
        ImGui::TextColored(palette.bronze, "PLAYER OPTIONS");
        bool godMode = gameplay.GodMode();
        if (ImGui::Checkbox("God Mode", &godMode)) gameplay.SetGodMode(godMode);

        bool superJump = gameplay.SuperJump();
        if (ImGui::Checkbox("Super Jump", &superJump)) gameplay.SetSuperJump(superJump);

        bool infiniteOxygen = gameplay.InfiniteOxygen();
        if (ImGui::Checkbox("Infinite Oxygen", &infiniteOxygen)) gameplay.SetInfiniteOxygen(infiniteOxygen);

        bool noRagdoll = gameplay.NoRagdoll();
        if (ImGui::Checkbox("No Ragdoll", &noRagdoll)) gameplay.SetNoRagdoll(noRagdoll);

        bool keepClean = gameplay.KeepPlayerClean();
        if (ImGui::Checkbox("Keep Player Clean", &keepClean)) gameplay.SetKeepPlayerClean(keepClean);

        Themes::Menu_Theme_Manager::Instance().DrawDivider();
        ImGui::TextColored(palette.bronze, "WANTED");

        int wantedLevel = SelfWantedLevelSelection();
        ImGui::SetNextItemWidth(420.0F);
        if (ImGui::SliderInt("Wanted Level", &wantedLevel, 0, 5))
            SetSelfWantedLevelSelection(wantedLevel);
        if (ImGui::Button("SET WANTED LEVEL", ImVec2(190.0F, 36.0F)))
            RequestSelfWantedLevel();
        ImGui::SameLine();
        ImGui::TextDisabled("Setting a level disables Never Wanted.");

        bool neverWanted = gameplay.NeverWanted();
        if (ImGui::Checkbox("Never Wanted", &neverWanted)) gameplay.SetNeverWanted(neverWanted);

        Themes::Menu_Theme_Manager::Instance().DrawDivider();
        ImGui::TextColored(palette.bronze, "MOVEMENT");

        bool fastRun = gameplay.FastRun();
        if (ImGui::Checkbox("Fast Run", &fastRun)) gameplay.SetFastRun(fastRun);
        float runSpeed = gameplay.RunSpeed();
        ImGui::BeginDisabled(!fastRun);
        ImGui::SetNextItemWidth(420.0F);
        if (ImGui::SliderFloat("Run / Sprint Speed", &runSpeed, 1.0F, 1.49F, "%.2fx"))
            gameplay.SetRunSpeed(runSpeed);
        ImGui::EndDisabled();

        bool fastSwim = gameplay.FastSwim();
        if (ImGui::Checkbox("Fast Swim", &fastSwim)) gameplay.SetFastSwim(fastSwim);
        float swimSpeed = gameplay.SwimSpeed();
        ImGui::BeginDisabled(!fastSwim);
        ImGui::SetNextItemWidth(420.0F);
        if (ImGui::SliderFloat("Swim Speed", &swimSpeed, 1.0F, 1.49F, "%.2fx"))
            gameplay.SetSwimSpeed(swimSpeed);
        ImGui::EndDisabled();

        bool unlimitedStamina = SelfUnlimitedStamina();
        if (ImGui::Checkbox("Unlimited Stamina", &unlimitedStamina))
            SetSelfUnlimitedStamina(unlimitedStamina);

        bool stealthSpeed = SelfStealthSpeed();
        if (ImGui::Checkbox("Stealth Speed", &stealthSpeed))
            SetSelfStealthSpeed(stealthSpeed);
        float stealthMultiplier = SelfStealthSpeedMultiplier();
        ImGui::BeginDisabled(!stealthSpeed);
        ImGui::SetNextItemWidth(420.0F);
        if (ImGui::SliderFloat("Stealth Movement Speed", &stealthMultiplier, 1.0F, 1.49F, "%.2fx"))
            SetSelfStealthSpeedMultiplier(stealthMultiplier);
        ImGui::EndDisabled();
        ImGui::TextDisabled("Stealth speed is applied only while the local ped is actually in stealth mode. Fast Run takes priority when both are enabled.");

        Themes::Menu_Theme_Manager::Instance().DrawDivider();
        ImGui::TextColored(palette.bronze, "SPECIAL ABILITY");

        bool specialAbilities = SelfSpecialAbilities();
        if (ImGui::Checkbox("Enable Special Abilities", &specialAbilities))
            SetSelfSpecialAbilities(specialAbilities);

        int specialSelection = SelfSpecialAbilitySelection();
        ImGui::BeginDisabled(!specialAbilities);
        ImGui::SetNextItemWidth(420.0F);
        if (ImGui::BeginCombo("Ability", SelfSpecialAbilityLabel(specialSelection))) {
            for (int index = 0; index < GTA_Self_Special_Ability_Count; ++index) {
                const bool selected = index == specialSelection;
                if (ImGui::Selectable(SelfSpecialAbilityLabel(index), selected)) {
                    specialSelection = index;
                    SetSelfSpecialAbilitySelection(index);
                }
                if (selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::EndDisabled();
        ImGui::TextDisabled("Experimental multiplayer special-ability path. Available types: Slipstream, Deadeye, Trevor Rage, Snapshot, and Insult.");

        Themes::Menu_Theme_Manager::Instance().DrawDivider();
        ImGui::TextColored(palette.bronze, "ONLINE");

        bool offTheRadar = gameplay.OffTheRadar();
        if (ImGui::Checkbox("Off The Radar", &offTheRadar))
            gameplay.SetOffTheRadar(offTheRadar);

        bool noIdleKick = SelfNoIdleKick();
        if (ImGui::Checkbox("No Idle Kick", &noIdleKick))
            SetSelfNoIdleKick(noIdleKick);
        if (noIdleKick) {
            ImGui::SameLine();
            ImGui::TextDisabled(SelfNoIdleKickReady() ? "Idle timers overridden" : "Resolving current-build idle tunables...");
        }

        Themes::Menu_Theme_Manager::Instance().DrawDivider();
        ImGui::TextColored(palette.bronze, "ACTIONS");

        if (ImGui::Button("SUICIDE", ImVec2(150.0F, 38.0F)))
            RequestSelfSuicide();
        ImGui::SameLine();
        if (ImGui::Button("SKIP CUTSCENE", ImVec2(190.0F, 38.0F)))
            gameplay.RequestSkipCutscene();

        ImGui::TextDisabled("Self actions execute on the validated GTA game thread. Weapon controls remain on the WEAPONS page.");
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("OUTFIT EDITOR")) {
        DrawOutfitEditorPage();
        ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
}
}
