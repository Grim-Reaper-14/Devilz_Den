    if (ImGui::BeginTabItem("GARAGE")) {
        ImGui::TextColored(Bronze, "PERSONAL VEHICLE");
        const auto pvStatus = Devilz::Integrations::GTA5_Enhanced::PersonalVehicleRequestStatus();
        auto pvListStatus = Devilz::Integrations::GTA5_Enhanced::PersonalVehicleListStatus();
        if (pvListStatus == Devilz::Integrations::GTA5_Enhanced::GTA_Personal_Vehicle_List_Status::Idle) {
            Devilz::Integrations::GTA5_Enhanced::RequestPersonalVehicleListRefresh();
            pvListStatus = Devilz::Integrations::GTA5_Enhanced::PersonalVehicleListStatus();
        }
        const bool pvBusy = pvStatus == Devilz::Integrations::GTA5_Enhanced::GTA_Personal_Vehicle_Request_Status::Queued ||
            pvStatus == Devilz::Integrations::GTA5_Enhanced::GTA_Personal_Vehicle_Request_Status::Requesting;

        static std::uint64_t personalVehicleGeneration = 0;
        static std::vector<Devilz::Integrations::GTA5_Enhanced::GTA_Personal_Vehicle_Info> personalVehicles;
        static int selectedPersonalVehicleId = -1;
        const auto currentGeneration = Devilz::Integrations::GTA5_Enhanced::PersonalVehicleGeneration();
        if (personalVehicleGeneration != currentGeneration) {
            personalVehicleGeneration = currentGeneration;
            personalVehicles = Devilz::Integrations::GTA5_Enhanced::PersonalVehicleSnapshot();
            const bool selectionStillValid = std::any_of(personalVehicles.begin(), personalVehicles.end(),
                [](const auto& vehicle) { return vehicle.id == selectedPersonalVehicleId; });
            if (!selectionStillValid)
                selectedPersonalVehicleId = personalVehicles.empty() ? -1 : personalVehicles.front().id;
        }

        const auto selectedPersonalVehicle = std::find_if(personalVehicles.begin(), personalVehicles.end(),
            [](const auto& vehicle) { return vehicle.id == selectedPersonalVehicleId; });
        std::string personalVehiclePreview = "No owned vehicles loaded";
        if (selectedPersonalVehicle != personalVehicles.end()) {
            personalVehiclePreview = selectedPersonalVehicle->displayName;
            if (!selectedPersonalVehicle->plateText.empty())
                personalVehiclePreview += " [" + selectedPersonalVehicle->plateText + "]";
        }

        ImGui::SetNextItemWidth(440.0F);
        if (ImGui::BeginCombo("Owned Personal Vehicle", personalVehiclePreview.c_str())) {
            for (const auto& vehicle : personalVehicles) {
                std::string label = vehicle.displayName;
                if (!vehicle.plateText.empty())
                    label += " [" + vehicle.plateText + "]";
                label += "  (Slot " + std::to_string(vehicle.id) + ")";
                const bool selected = vehicle.id == selectedPersonalVehicleId;
                if (ImGui::Selectable(label.c_str(), selected))
                    selectedPersonalVehicleId = vehicle.id;
                if (selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        if (ImGui::Button("REFRESH OWNED VEHICLES", ImVec2(210.0F, 34.0F)))
            Devilz::Integrations::GTA5_Enhanced::RequestPersonalVehicleListRefresh();
        ImGui::SameLine();
        switch (pvListStatus) {
        case Devilz::Integrations::GTA5_Enhanced::GTA_Personal_Vehicle_List_Status::Idle:
        case Devilz::Integrations::GTA5_Enhanced::GTA_Personal_Vehicle_List_Status::Queued:
            ImGui::TextDisabled("Queued..."); break;
        case Devilz::Integrations::GTA5_Enhanced::GTA_Personal_Vehicle_List_Status::Scanning:
            ImGui::TextDisabled("Scanning owned vehicles..."); break;
        case Devilz::Integrations::GTA5_Enhanced::GTA_Personal_Vehicle_List_Status::Ready:
            ImGui::Text("%zu owned vehicle%s", personalVehicles.size(), personalVehicles.size() == 1 ? "" : "s"); break;
        case Devilz::Integrations::GTA5_Enhanced::GTA_Personal_Vehicle_List_Status::Unavailable:
            ImGui::TextColored(EmberRed, "Enter GTA Online freemode."); break;
        case Devilz::Integrations::GTA5_Enhanced::GTA_Personal_Vehicle_List_Status::Failed:
            ImGui::TextColored(EmberRed, "Owned vehicle scan failed."); break;
        }

        ImGui::BeginDisabled(pvBusy || selectedPersonalVehicleId < 0);
        if (ImGui::Button("REQUEST SELECTED VEHICLE", ImVec2(230.0F, 38.0F)))
            Devilz::Integrations::GTA5_Enhanced::RequestPersonalVehicle(selectedPersonalVehicleId);
        ImGui::EndDisabled();

        ImGui::SameLine();
        ImGui::BeginDisabled(pvBusy);
        if (ImGui::Button("REQUEST LAST SAVED", ImVec2(190.0F, 38.0F)))
            Devilz::Integrations::GTA5_Enhanced::RequestCurrentPersonalVehicle();
        ImGui::EndDisabled();

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
        ImGui::TextDisabled("Select any owned MPSV vehicle by garage slot, or fall back to GTA's last-saved personal vehicle.");

        MedievalDivider();
        ImGui::TextColored(Bronze, "SAVE VEHICLE TO GARAGE");
        const auto personalSaveStatus = Devilz::Integrations::GTA5_Enhanced::VehiclePersonalSaveStatus();
        const bool personalSaveBusy =
            personalSaveStatus == Devilz::Integrations::GTA5_Enhanced::GTA_Vehicle_Personal_Save_Status::Queued ||
            personalSaveStatus == Devilz::Integrations::GTA5_Enhanced::GTA_Vehicle_Personal_Save_Status::Validating ||
            personalSaveStatus == Devilz::Integrations::GTA5_Enhanced::GTA_Vehicle_Personal_Save_Status::OpeningGarageMenu ||
            personalSaveStatus == Devilz::Integrations::GTA5_Enhanced::GTA_Vehicle_Personal_Save_Status::WaitingForGarageSelection;
        const bool personalSaveReady = Devilz::Integrations::GTA5_Enhanced::VehiclePersonalSaveRuntimeReady();

        ImGui::BeginDisabled(forge.vehicle == 0 || personalSaveBusy || !personalSaveReady);
        if (ImGui::Button("SAVE CURRENT VEHICLE TO GARAGE", ImVec2(285.0F, 38.0F)))
            Devilz::Integrations::GTA5_Enhanced::RequestSaveCurrentVehicleToGarage();
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (personalSaveStatus == Devilz::Integrations::GTA5_Enhanced::GTA_Vehicle_Personal_Save_Status::InvalidVehicle ||
            personalSaveStatus == Devilz::Integrations::GTA5_Enhanced::GTA_Vehicle_Personal_Save_Status::NoVehicle ||
            personalSaveStatus == Devilz::Integrations::GTA5_Enhanced::GTA_Vehicle_Personal_Save_Status::Unavailable ||
            personalSaveStatus == Devilz::Integrations::GTA5_Enhanced::GTA_Vehicle_Personal_Save_Status::Failed) {
            ImGui::TextColored(EmberRed, "%s", PersonalVehicleSaveStatusText(personalSaveStatus));
        } else {
            ImGui::TextDisabled("%s", PersonalVehicleSaveStatusText(personalSaveStatus));
        }
        ImGui::TextDisabled(
            "Uses GTA Online's AM_MP_VEHICLE_REWARD transaction. GTA opens its own garage selection so you choose the garage/slot in-game.");

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

        bool removeRestrictions = Devilz::Integrations::GTA5_Enhanced::RemoveLSCRestrictions();
        if (ImGui::Checkbox("Remove LSC Restrictions", &removeRestrictions))
            Devilz::Integrations::GTA5_Enhanced::SetRemoveLSCRestrictions(removeRestrictions);

        ImGui::SameLine();
        switch (Devilz::Integrations::GTA5_Enhanced::RemoveLSCRestrictionsStatus()) {
        case Devilz::Integrations::GTA5_Enhanced::GTA_Vehicle_LSC_Restriction_Status::Disabled:
            ImGui::TextDisabled("Disabled");
            break;
        case Devilz::Integrations::GTA5_Enhanced::GTA_Vehicle_LSC_Restriction_Status::WaitingForScript:
            ImGui::TextDisabled("Waiting for LSC...");
            break;
        case Devilz::Integrations::GTA5_Enhanced::GTA_Vehicle_LSC_Restriction_Status::Active:
            ImGui::Text("Active");
            break;
        case Devilz::Integrations::GTA5_Enhanced::GTA_Vehicle_LSC_Restriction_Status::Unsupported:
            ImGui::TextColored(EmberRed, "Unsupported script build/signature.");
            break;
        }
        ImGui::TextDisabled("Reversibly removes the two current carmod_shop modification restrictions while enabled.");

        ImGui::TextWrapped(
            "LSC Prep repairs the current vehicle, fixes deformation, restores engine/body health, cleans it, starts the engine, "
            "and refreshes Vehicle Editor. Remove LSC Restrictions changes local carmod_shop checks only. SAVE CURRENT VEHICLE TO GARAGE "
            "uses GTA's own vehicle-reward transaction to create the personal-vehicle/garage flow.");
        ImGui::TextDisabled("Spawned/custom vehicles can still be kept in Saved Builds for later use.");

        ImGui::EndTabItem();
    }
