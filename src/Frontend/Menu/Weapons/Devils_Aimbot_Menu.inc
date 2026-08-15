#include "Integrations/GTA5_Enhanced/Runtime/Devils_Aimbot.hpp"

namespace Devilz::Frontend
{
void Devils_Den_Menu::DrawWeaponsPage()
{
    DrawWeaponsPageLegacy();

    ImGui::Separator();
    ImGui::TextColored(ImVec4{0.78F, 0.66F, 0.44F, 1.00F}, "DEVILS_AIMBOT");

    const bool available = Integrations::GTA5_Enhanced::DevilsAimbotAvailable();
    bool enabled = Integrations::GTA5_Enhanced::DevilsAimbotEnabled();
    bool aimForHead = Integrations::GTA5_Enhanced::DevilsAimbotAimForHead();
    bool targetDrivers = Integrations::GTA5_Enhanced::DevilsAimbotTargetDrivers();

    ImGui::BeginDisabled(!available);
    if (ImGui::Checkbox("Devils_Aimbot", &enabled))
        Integrations::GTA5_Enhanced::SetDevilsAimbotEnabled(enabled);
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::BeginDisabled(!available || !enabled);
    if (ImGui::Checkbox("Aim For Head", &aimForHead))
        Integrations::GTA5_Enhanced::SetDevilsAimbotAimForHead(aimForHead);
    ImGui::SameLine();
    if (ImGui::Checkbox("Target Drivers", &targetDrivers))
        Integrations::GTA5_Enhanced::SetDevilsAimbotTargetDrivers(targetDrivers);
    ImGui::EndDisabled();

    if (!available)
        ImGui::TextColored(ImVec4{0.88F, 0.10F, 0.045F, 1.00F}, "%s", Integrations::GTA5_Enhanced::DevilsAimbotStatusText());
    else
        ImGui::TextDisabled("GTA assisted aim / lock-on patches. No per-frame world scan. %s",
            Integrations::GTA5_Enhanced::DevilsAimbotStatusText());
}
}
