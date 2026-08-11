#pragma once

#include "Frontend/Menu/Themes/Menu_Theme.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Gameplay_State.hpp"

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
    ImGui::TextDisabled("- live Story Mode features");
    Themes::Menu_Theme_Manager::Instance().DrawDivider();

    ImGui::TextColored(palette.bronze, "PLAYER OPTIONS");
    bool godMode = gameplay.GodMode();
    if (ImGui::Checkbox("God Mode", &godMode)) gameplay.SetGodMode(godMode);

    bool neverWanted = gameplay.NeverWanted();
    if (ImGui::Checkbox("Never Wanted", &neverWanted)) gameplay.SetNeverWanted(neverWanted);

    bool superJump = gameplay.SuperJump();
    if (ImGui::Checkbox("Super Jump", &superJump)) gameplay.SetSuperJump(superJump);

    bool infiniteOxygen = gameplay.InfiniteOxygen();
    if (ImGui::Checkbox("Infinite Oxygen", &infiniteOxygen)) gameplay.SetInfiniteOxygen(infiniteOxygen);

    bool noRagdoll = gameplay.NoRagdoll();
    if (ImGui::Checkbox("No Ragdoll", &noRagdoll)) gameplay.SetNoRagdoll(noRagdoll);

    bool keepClean = gameplay.KeepPlayerClean();
    if (ImGui::Checkbox("Keep Player Clean", &keepClean)) gameplay.SetKeepPlayerClean(keepClean);

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

    ImGui::TextDisabled("Movement multipliers restore to 1.00x when disabled. Fast Run also applies the live ped move-rate override each game tick.");
    Themes::Menu_Theme_Manager::Instance().DrawDivider();
    ImGui::TextDisabled("Weapon controls live on the WEAPONS page. Live features execute on the validated game thread.");
}
}
