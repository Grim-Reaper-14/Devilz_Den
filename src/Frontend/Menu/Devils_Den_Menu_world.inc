void Devils_Den_Menu::DrawWorldPage()
{
    using namespace Devilz::Integrations::GTA5_Enhanced;

    ImGui::TextColored(EmberRed, "WORLD");
    ImGui::SameLine();
    ImGui::TextDisabled("- environment controls");
    MedievalDivider();

    ImGui::TextColored(Bronze, "NETWORK TIME");
    int hour = NetworkTimeHour();
    int minute = NetworkTimeMinute();
    int second = NetworkTimeSecond();

    ImGui::SetNextItemWidth(260.0F);
    if (ImGui::SliderInt("Hour", &hour, 0, 23))
        SetNetworkTimeSelection(hour, minute, second);

    ImGui::SetNextItemWidth(260.0F);
    if (ImGui::SliderInt("Minute", &minute, 0, 59))
        SetNetworkTimeSelection(hour, minute, second);

    ImGui::SetNextItemWidth(260.0F);
    if (ImGui::SliderInt("Second", &second, 0, 59))
        SetNetworkTimeSelection(hour, minute, second);

    if (ImGui::Button("SET NETWORK TIME", ImVec2(190.0F, 38.0F)))
        RequestSetNetworkTime();

    ImGui::SameLine();
    bool freezeTime = FreezeNetworkTime();
    if (ImGui::Checkbox("Freeze Network Time", &freezeTime))
        SetFreezeNetworkTime(freezeTime);

    ImGui::TextDisabled("Selected: %02d:%02d:%02d", NetworkTimeHour(), NetworkTimeMinute(), NetworkTimeSecond());
    ImGui::TextDisabled("Turning Freeze off clears the network clock override.");

    MedievalDivider();
    ImGui::TextColored(Bronze, "WEATHER");

    int weather = WorldWeatherSelection();
    ImGui::SetNextItemWidth(300.0F);
    if (ImGui::BeginCombo("Weather", WorldWeatherLabel(weather))) {
        for (int index = 0; index < GTA_World_Weather_Count; ++index) {
            const bool selected = weather == index;
            if (ImGui::Selectable(WorldWeatherLabel(index), selected)) {
                weather = index;
                SetWorldWeatherSelection(index);
            }
            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    if (ImGui::Button("SET WEATHER", ImVec2(150.0F, 38.0F)))
        RequestSetWorldWeather();

    ImGui::SameLine();
    bool forceWeather = ForceWorldWeather();
    if (ImGui::Checkbox("Force Weather", &forceWeather))
        SetForceWorldWeather(forceWeather);

    ImGui::SameLine();
    if (ImGui::Button("RESET WEATHER", ImVec2(150.0F, 38.0F)))
        RequestResetWorldWeather();

    ImGui::TextDisabled("Weather overrides are local to your game. Force Weather reapplies the selected type every tick.");
}

void Devils_Den_Menu::DrawPlaceholderPage(const char* title, const char* detail)
{
    if (title && std::string_view(title) == "WORLD") {
        DrawWorldPage();
        return;
    }

    DrawPlaceholderPageLegacy(title, detail);
}
