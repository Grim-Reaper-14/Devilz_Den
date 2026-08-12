    if (ImGui::BeginTabItem("GARAGE")) {
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
