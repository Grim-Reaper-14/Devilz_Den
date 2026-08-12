void Devils_Den_Menu::DrawSelfPage()
{
    DrawSelfPageLegacy();

    auto& gameplay = GTA_Gameplay_State::Instance();

    MedievalDivider();
    ImGui::TextColored(Bronze, "ONLINE");
    bool offTheRadar = gameplay.OffTheRadar();
    if (ImGui::Checkbox("Off The Radar", &offTheRadar))
        gameplay.SetOffTheRadar(offTheRadar);
    ImGui::TextDisabled("Hides your freemode radar blip while enabled.");

    MedievalDivider();
    ImGui::TextColored(Bronze, "ACTIONS");
    if (ImGui::Button("SKIP CUTSCENE", ImVec2(190.0F, 38.0F)))
        gameplay.RequestSkipCutscene();
    ImGui::SameLine();
    ImGui::TextDisabled("Stops the currently playing cutscene immediately.");
}
