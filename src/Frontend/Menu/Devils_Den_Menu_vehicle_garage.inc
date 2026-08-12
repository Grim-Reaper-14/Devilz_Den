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
        ImGui::TextColored(Bronze, "LSC PREP");
        ImGui::TextDisabled("Prepares the vehicle you are currently driving for a normal Los Santos Customs visit.");

        const auto prepStatus = Devilz::Integrations::GTA5_Enhanced::VehicleLSCPrepStatus();
        const bool prepBusy = prepStatus == Devilz::Integrations::GTA5_Enhanced::GTA_Vehicle_LSC_Prep_Status::Queued ||
            prepStatus == Devilz::Integrations::GTA5_Enhanced::GTA_Vehicle_LSC_Prep_Status::Preparing;

        ImGui::BeginDisabled(forge.vehicle == 0 || prepBusy);
        if (ImGui::Button("PREP CURRENT VEHICLE FOR LSC", ImVec2(270.0F, 38.0F)))
            Devilz::Integrations::GTA5_Enhanced::RequestVehicleLSCPrep();
        ImGui::EndDisabled();

        ImGui::SameLine();
        switch (prepStatus) {
        case Devilz::Integrations::GTA5_Enhanced::GTA_Vehicle_LSC_Prep_Status::Idle:
            ImGui::TextDisabled("Ready");
            break;
        case Devilz::Integrations::GTA5_Enhanced::GTA_Vehicle_LSC_Prep_Status::Queued:
            ImGui::TextDisabled("Queued...");
            break;
        case Devilz::Integrations::GTA5_Enhanced::GTA_Vehicle_LSC_Prep_Status::Preparing:
            ImGui::TextDisabled("Preparing vehicle...");
            break;
        case Devilz::Integrations::GTA5_Enhanced::GTA_Vehicle_LSC_Prep_Status::Prepared:
            ImGui::Text("Vehicle prepared.");
            break;
        case Devilz::Integrations::GTA5_Enhanced::GTA_Vehicle_LSC_Prep_Status::NoVehicle:
            ImGui::TextColored(EmberRed, "Get in the vehicle you want to prep.");
            break;
        case Devilz::Integrations::GTA5_Enhanced::GTA_Vehicle_LSC_Prep_Status::Failed:
            ImGui::TextColored(EmberRed, "LSC prep failed.");
            break;
        }

        ImGui::TextWrapped(
            "LSC Prep repairs the current vehicle, fixes deformation, restores engine/body health, cleans it, starts the engine, "
            "and refreshes Devils Forge. It does not create ownership, insurance, or a garage slot; GTA handles those through its normal flow.");
        ImGui::TextDisabled("Spawned/custom vehicles can still be kept in Saved Builds for later use.");

        ImGui::EndTabItem();
    }
