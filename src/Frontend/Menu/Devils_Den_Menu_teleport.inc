namespace Devilz::Frontend
{
void Devils_Den_Menu::DrawTeleportPage()
{
    using namespace Integrations::GTA5_Enhanced;

    auto& gameplay = GTA_Gameplay_State::Instance();

    ImGui::TextColored(EmberRed, "TELEPORT");
    ImGui::SameLine();
    ImGui::TextDisabled("- waypoint, objective and destination travel");
    MedievalDivider();

    const auto waypointStatus = gameplay.TeleportStatus();
    const bool waypointBusy = waypointStatus == GTA_Teleport_Waypoint_Status::Queued ||
        waypointStatus == GTA_Teleport_Waypoint_Status::Resolving;
    const auto quickStatus = QuickTeleportStatus();
    const bool quickBusy = quickStatus == GTA_Teleport_Quick_Status::Queued;
    const bool teleportBusy = waypointBusy || quickBusy;

    ImGui::TextColored(Bronze, "QUICK TRAVEL");
    ImGui::TextDisabled("Waypoint travel resolves terrain before moving you. Objective travel follows the active mission/objective map blip.");

    ImGui::BeginDisabled(teleportBusy);
    if (ImGui::Button("TELEPORT TO WAYPOINT", ImVec2(240.0F, 42.0F)))
        gameplay.RequestTeleportToWaypoint();
    ImGui::SameLine();
    if (ImGui::Button("TELEPORT TO OBJECTIVE", ImVec2(240.0F, 42.0F)))
        RequestTeleportToObjective();
    ImGui::EndDisabled();

    bool autoTeleport = AutoTeleportToWaypoint();
    if (ImGui::Checkbox("Auto Teleport to Waypoint", &autoTeleport))
        SetAutoTeleportToWaypoint(autoTeleport);
    ImGui::SameLine();
    ImGui::TextDisabled("Teleports once when a waypoint is placed or moved.");

    ImGui::TextColored(Bronze, "WAYPOINT STATUS");
    ImGui::SameLine();
    ImGui::TextUnformatted(TeleportStatusText(waypointStatus));

    MedievalDivider();
    ImGui::TextColored(Bronze, "ONLINE DESTINATIONS");
    ImGui::TextDisabled("Uses the live GTA map icon, so the destination must be available in the current session.");

    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float halfWidth = (ImGui::GetContentRegionAvail().x - spacing) * 0.5F;
    ImGui::BeginDisabled(teleportBusy);
    if (ImGui::Button("LS CAR MEET", ImVec2(halfWidth, 38.0F)))
        RequestQuickTeleport(GTA_Teleport_Quick_Destination::LsCarMeet);
    ImGui::SameLine();
    if (ImGui::Button("ARENA GARAGE", ImVec2(halfWidth, 38.0F)))
        RequestQuickTeleport(GTA_Teleport_Quick_Destination::ArenaGarage);
    ImGui::EndDisabled();

    MedievalDivider();
    ImGui::TextColored(Bronze, "SHOPS");
    ImGui::TextDisabled("Teleports to the closest active shop blip of the selected type.");

    const float thirdWidth = (ImGui::GetContentRegionAvail().x - spacing * 2.0F) / 3.0F;
    ImGui::BeginDisabled(teleportBusy);
    if (ImGui::Button("CLOTHING SHOP", ImVec2(thirdWidth, 38.0F)))
        RequestQuickTeleport(GTA_Teleport_Quick_Destination::ClothingShop);
    ImGui::SameLine();
    if (ImGui::Button("GUN SHOP", ImVec2(thirdWidth, 38.0F)))
        RequestQuickTeleport(GTA_Teleport_Quick_Destination::GunShop);
    ImGui::SameLine();
    if (ImGui::Button("TATTOO SHOP", ImVec2(thirdWidth, 38.0F)))
        RequestQuickTeleport(GTA_Teleport_Quick_Destination::TattooShop);
    ImGui::EndDisabled();

    ImGui::TextColored(Bronze, "QUICK STATUS");
    ImGui::SameLine();
    switch (quickStatus) {
    case GTA_Teleport_Quick_Status::Idle:
        ImGui::TextDisabled("Ready");
        break;
    case GTA_Teleport_Quick_Status::Queued:
        ImGui::TextDisabled("Queued...");
        break;
    case GTA_Teleport_Quick_Status::Succeeded:
        ImGui::Text("Teleport complete");
        break;
    case GTA_Teleport_Quick_Status::NoDestination:
        ImGui::TextColored(EmberRed, "Destination is not active on the map.");
        break;
    case GTA_Teleport_Quick_Status::Failed:
        ImGui::TextColored(EmberRed, "Teleport failed.");
        break;
    }

    MedievalDivider();
    ImGui::TextColored(Bronze, "PLACES");

    const float placeWidth = (ImGui::GetContentRegionAvail().x - spacing) * 0.5F;
    ImGui::BeginDisabled(teleportBusy);
    for (std::size_t index = 0; index < GTA_Teleport_Locations.size(); ++index) {
        const auto& location = GTA_Teleport_Locations[index];
        ImGui::PushID(static_cast<int>(index));
        if (ImGui::Button(location.label.data(), ImVec2(placeWidth, 34.0F)))
            gameplay.RequestTeleportToLocation(location.id);
        ImGui::PopID();
        if ((index % 2U) == 0U)
            ImGui::SameLine();
    }
    ImGui::EndDisabled();

    ImGui::TextDisabled("When seated in a vehicle, teleport actions move the occupied vehicle with you.");
}
}
