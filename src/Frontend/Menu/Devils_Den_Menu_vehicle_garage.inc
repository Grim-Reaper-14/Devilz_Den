    if (ImGui::BeginTabItem("GARAGE")) {
        ImGui::TextColored(Bronze, "PERSONAL VEHICLE");
        const auto pvStatus = Devilz::Integrations::GTA5_Enhanced::PersonalVehicleRequestStatus();
        const bool pvBusy = pvStatus == Devilz::Integrations::GTA5_Enhanced::GTA_Personal_Vehicle_Request_Status::Queued ||
            pvStatus == Devilz::Integrations::GTA5_Enhanced::GTA_Personal_Vehicle_Request_Status::Requesting;

        ImGui::BeginDisabled(pvBusy);
        if (ImGui::Button("REQUEST PERSONAL VEHICLE", ImVec2(230.0F, 38.0F)))
            Devilz::Integrations::GTA5_Enhanced::RequestCurrentPersonalVehicle();
        ImGui::EndDisabled();

        ImGui::SameLine();
        switch (pvStatus) {
        case Devilz::Integrations::GTA5_Enhanced::GTA_Personal_Vehicle_Request_Status::Idle:
            ImGui::TextDisabled("Ready");
            break;
        case Devilz::Integrations::GTA5_Enhanced::GTA_Personal_Vehicle_Request_Status::Queued:
            ImGui::TextDisabled("Queued...");
            break;
        case Devilz::Integrations::GTA5_Enhanced::GTA_Personal_Vehicle_Request_Status::Requesting:
            ImGui::TextDisabled("Requesting...");
            break;
        case Devilz::Integrations::GTA5_Enhanced::GTA_Personal_Vehicle_Request_Status::Requested:
            ImGui::Text("Request sent.");
            break;
        case Devilz::Integrations::GTA5_Enhanced::GTA_Personal_Vehicle_Request_Status::Unavailable:
            ImGui::TextColored(EmberRed, "Enter GTA Online freemode.");
            break;
        case Devilz::Integrations::GTA5_Enhanced::GTA_Personal_Vehicle_Request_Status::Busy:
            ImGui::TextColored(EmberRed, "Another PV request is active.");
            break;
        case Devilz::Integrations::GTA5_Enhanced::GTA_Personal_Vehicle_Request_Status::Failed:
            ImGui::TextColored(EmberRed, "Personal vehicle request failed.");
            break;
        }
        ImGui::TextDisabled("Requests the current/last-saved owned personal vehicle through freemode.");

        MedievalDivider();
        ImGui::TextColored(Bronze, "GTA GARAGE SAVE");
        ImGui::TextDisabled("Runs through the validated game-thread request path.");

        const auto garageStatus = Devilz::Integrations::GTA5_Enhanced::VehicleGarageSaveStatus();
        const bool garageBusy = garageStatus == Devilz::Integrations::GTA5_Enhanced::GTA_Vehicle_Garage_Status::Queued ||
            garageStatus == Devilz::Integrations::GTA5_Enhanced::GTA_Vehicle_Garage_Status::Saving;

        ImGui::BeginDisabled(forge.vehicle == 0 || garageBusy);
        if (ImGui::Button("SAVE TO GTA GARAGE", ImVec2(220.0F, 38.0F)))
            Devilz::Integrations::GTA5_Enhanced::RequestVehicleGarageSave();
        ImGui::EndDisabled();

        ImGui::SameLine();
        switch (garageStatus) {
        case Devilz::Integrations::GTA5_Enhanced::GTA_Vehicle_Garage_Status::Idle:
            ImGui::TextDisabled("Ready");
            break;
        case Devilz::Integrations::GTA5_Enhanced::GTA_Vehicle_Garage_Status::Queued:
            ImGui::TextDisabled("Queued...");
            break;
        case Devilz::Integrations::GTA5_Enhanced::GTA_Vehicle_Garage_Status::Saving:
            ImGui::TextDisabled("Saving vehicle...");
            break;
        case Devilz::Integrations::GTA5_Enhanced::GTA_Vehicle_Garage_Status::Saved:
            ImGui::Text("Vehicle saved.");
            break;
        case Devilz::Integrations::GTA5_Enhanced::GTA_Vehicle_Garage_Status::OwnershipRequired:
            ImGui::TextColored(EmberRed, "Ownership validation required.");
            break;
        case Devilz::Integrations::GTA5_Enhanced::GTA_Vehicle_Garage_Status::Failed:
            ImGui::TextColored(EmberRed, "Garage save failed.");
            break;
        }

        MedievalDivider();
        ImGui::TextWrapped(
            "This scaffold only persists a vehicle after a legitimate personal-vehicle validator is connected. "
            "It does not inject ownership or fabricate a garage slot for spawned/unowned vehicles.");

        ImGui::EndTabItem();
    }
