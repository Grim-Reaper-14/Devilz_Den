            if (entry.modelHash != m_selectedVehicleModel)
                continue;
            const bool classMatches = m_vehicleClassFilter < 0 || entry.vehicleClass == m_vehicleClassFilter;
            const bool searchMatches = ContainsInsensitive(entry.displayName, search) ||
                ContainsInsensitive(entry.makeName, search) || ContainsInsensitive(entry.modelName, search);
            if (classMatches && searchMatches)
                selected = &entry;
            break;
        }
        if (!selected && firstVisible) {
            m_selectedVehicleModel = firstVisible->modelHash;
            selected = firstVisible;
        }

        ImGui::SameLine();
        if (ImGui::BeginChild("##VehicleDetails", ImVec2(0.0F, 270.0F), ImGuiChildFlags_Borders)) {
            ImGui::TextColored(Bronze, "SELECTED VEHICLE");
            if (selected) {
                ImGui::TextWrapped("%s", selected->displayName.c_str());
                if (!selected->makeName.empty()) ImGui::TextDisabled("%s", selected->makeName.c_str());
                ImGui::Text("Model: %s", selected->modelName.c_str());
                if (selected->vehicleClass >= 0 && selected->vehicleClass < static_cast<int>(GTA_Vehicle_Class_Names.size()))
                    ImGui::Text("Class: %s", GTA_Vehicle_Class_Names[static_cast<std::size_t>(selected->vehicleClass)].data());
                ImGui::Separator();
                ImGui::Checkbox("Spawn Inside", &m_spawnInsideVehicle);
                ImGui::Checkbox("Spawn Maxed", &m_spawnVehicleMaxed);
                ImGui::Checkbox("Place On Ground", &m_spawnVehicleOnGround);
                ImGui::Checkbox("Engine Running", &m_spawnVehicleEngineRunning);
                ImGui::Checkbox("Invincible", &m_spawnVehicleInvincible);
                ImGui::Checkbox("Clean", &m_spawnVehicleClean);
            } else {
                ImGui::TextDisabled("No vehicles match this filter/search.");
            }
        }
        ImGui::EndChild();

        ImGui::TextColored(Bronze, "STATUS");
        ImGui::SameLine();
        ImGui::TextUnformatted(VehicleSpawnStatusText(spawnStatus));
        ImGui::BeginDisabled(spawnBusy || !selected);
        if (ImGui::Button("SPAWN SELECTED VEHICLE", ImVec2(260.0F, 42.0F)) && selected) {
            GTA_Vehicle_Spawn_Options options{};
            options.spawnInside = m_spawnInsideVehicle;
            options.spawnMaxed = m_spawnVehicleMaxed;
            options.placeOnGround = m_spawnVehicleOnGround;
            options.engineRunning = m_spawnVehicleEngineRunning;
            options.invincible = m_spawnVehicleInvincible;
            options.clean = m_spawnVehicleClean;
            vehicles.RequestSpawn(selected->modelHash, options);
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::TextDisabled("Catalog: %zu Enhanced models", catalog.size());
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("VEHICLE EDITOR")) {
        ImGui::TextColored(Bronze, "VEHICLE TOOLS");
        const auto cloneNearestStatus = CloneNearestVehicleStatus();
        const bool cloneNearestBusy =
            cloneNearestStatus == GTA_Vehicle_Clone_Nearest_Status::Queued ||
            cloneNearestStatus == GTA_Vehicle_Clone_Nearest_Status::Searching ||
            cloneNearestStatus == GTA_Vehicle_Clone_Nearest_Status::SpawnQueued;

        ImGui::BeginDisabled(cloneNearestBusy || spawnBusy);
        if (ImGui::Button("CLONE NEAREST VEHICLE", ImVec2(220.0F, 36.0F)))
            RequestCloneNearestVehicle();
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::TextDisabled("%s", CloneNearestVehicleStatusText(cloneNearestStatus));

        ImGui::TextDisabled(
            "Clone Nearest copies the closest vehicle's live model, mods, paint, wheels, lights, extras and plate into a new spawned vehicle.");
        MedievalDivider();

        ImGui::TextColored(Bronze, "CURRENT VEHICLE");
        if (forge.vehicle == 0) {
            ImGui::TextDisabled("Enter a vehicle to unlock Vehicle Editor.");
            if (ImGui::Button("REFRESH VEHICLE DATA", ImVec2(210.0F, 34.0F)))
                vehicles.RequestForgeSnapshotRefresh();
        } else {
            if (currentMetadata) {
                if (!currentMetadata->makeName.empty())
                    ImGui::Text("%s %s", currentMetadata->makeName.c_str(), currentMetadata->displayName.c_str());
                else
                    ImGui::Text("%s", currentMetadata->displayName.c_str());
                ImGui::TextDisabled("Model: %s  |  Hash: 0x%08X", currentMetadata->modelName.c_str(), forge.modelHash);
            } else {
                ImGui::Text("Vehicle 0x%08X", forge.modelHash);
            }
            ImGui::SameLine();
            if (ImGui::Button("REFRESH", ImVec2(90.0F, 28.0F))) vehicles.RequestForgeSnapshotRefresh();

            MedievalDivider();
            ImGui::TextColored(Bronze, "CONDITION / PROTECTION");
            bool keepPerfect = vehicles.KeepVehiclePerfect();
            if (ImGui::Checkbox("Always Keep Vehicle Perfect", &keepPerfect)) vehicles.SetKeepVehiclePerfect(keepPerfect);
            ImGui::SameLine();
            ImGui::TextDisabled("Repairs damage + deformation and keeps dirt at zero");
            bool vehicleGodMode = vehicles.VehicleGodMode();
            if (ImGui::Checkbox("Vehicle God Mode", &vehicleGodMode)) vehicles.SetVehicleGodMode(vehicleGodMode);
            if (ImGui::Button("REPAIR EVERYTHING", ImVec2(170.0F, 34.0F)))
                vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::RepairVehicle});
            ImGui::SameLine();
            if (ImGui::Button("CLEAN VEHICLE", ImVec2(150.0F, 34.0F)))
                vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::CleanVehicle});
            ImGui::SameLine();
            if (ImGui::Checkbox("Quick Lowered Stance", &m_forgeLoweredStance))
                vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::SetLoweredStance, m_forgeLoweredStance ? 1 : 0});

            if (ImGui::Button("ENGINE ON", ImVec2(125.0F, 32.0F)))
                vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::SetEngineRunning, 1});
            ImGui::SameLine();
            if (ImGui::Button("ENGINE OFF", ImVec2(125.0F, 32.0F)))
                vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::SetEngineRunning, 0});
            ImGui::SameLine();
            if (ImGui::Button("PLACE ON GROUND", ImVec2(165.0F, 32.0F)))
                vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::PlaceOnGround});

            MedievalDivider();
            ImGui::TextColored(Bronze, "VEHICLE-SPECIFIC MODS");
            ImGui::TextDisabled("Only categories supported by this vehicle are shown. Options apply immediately.");
            if (forge.categories.empty()) {
                ImGui::TextColored(EmberRed, "Preparing vehicle mod data...");
                ImGui::TextDisabled("Mod kits: %d | scan attempt: %d / 12", forge.modKitCount, forge.modScanAttempts);
                if (forge.modScanAttempts >= 12)
                    ImGui::TextWrapped("GTA is still reporting zero supported LSC mod slots for this vehicle. Use RESCAN after entering another LSC-compatible car; the rest of Vehicle Editor remains available.");
                if (ImGui::Button("RESCAN MOD CATEGORIES", ImVec2(210.0F, 30.0F)))
                    vehicles.RequestForgeSnapshotRefresh();
            }

            const GTA_Vehicle_Forge_Category* selectedCategory = nullptr;
            for (const auto& category : forge.categories)
                if (category.slot == m_forgeModSlot) selectedCategory = &category;

            if (ImGui::BeginChild("##ForgeCategories", ImVec2(235.0F, 225.0F), ImGuiChildFlags_Borders)) {
                ImGui::TextColored(Bronze, "CATEGORIES");
                for (const auto& category : forge.categories) {
                    if (category.slot == 23 || category.slot == 24)
                        continue;
                    ImGui::PushID(category.slot);
                    if (ImGui::Selectable(category.name.c_str(), m_forgeModSlot == category.slot)) {
                        m_forgeModSlot = category.slot;
                        m_forgeModIndex = category.installedIndex;
                        selectedCategory = &category;
                    }
                    ImGui::PopID();
                }
            }
            ImGui::EndChild();
            ImGui::SameLine();
            if (ImGui::BeginChild("##ForgeOptions", ImVec2(0.0F, 225.0F), ImGuiChildFlags_Borders)) {
                ImGui::TextColored(Bronze, "OPTIONS");
                if (!selectedCategory) {
                    ImGui::TextDisabled(forge.categories.empty() ? "Waiting for vehicle mod scan." : "Choose a supported category.");
                } else {
                    ImGui::Text("%s", selectedCategory->name.c_str());
                    ImGui::Separator();
                    for (const auto& option : selectedCategory->options) {
                        const bool installed = option.index == selectedCategory->installedIndex;
                        ImGui::PushID(option.index);
                        if (ImGui::Selectable(option.name.c_str(), installed)) {
                            m_forgeModIndex = option.index;
                            vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::SetMod,
                                selectedCategory->slot, option.index, 0});
                        }
                        ImGui::PopID();
                    }
                }
            }
            ImGui::EndChild();

            if (ImGui::Button(forge.turboEnabled ? "TURBO: ON" : "TURBO: OFF", ImVec2(125.0F, 32.0F)))
                vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::ToggleMod, 18, forge.turboEnabled ? 0 : 1});
            ImGui::SameLine();
            if (ImGui::Button(forge.xenonEnabled ? "XENON: ON" : "XENON: OFF", ImVec2(125.0F, 32.0F)))
                vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::ToggleMod, 22, forge.xenonEnabled ? 0 : 1});
            ImGui::SameLine();
            if (ImGui::Button(forge.tireSmokeEnabled ? "TIRE SMOKE: ON" : "TIRE SMOKE: OFF", ImVec2(150.0F, 32.0F)))
                vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::ToggleMod, 20, forge.tireSmokeEnabled ? 0 : 1});

            MedievalDivider();
            ImGui::TextColored(Bronze, "LIGHTING");
            ImGui::SetNextItemWidth(280.0F);
            if (ImGui::BeginCombo("Headlight Color", LookupVehicleValue(GTA_Vehicle_Headlight_Colors, forge.xenonColor, "Default"))) {
                for (const auto& color : GTA_Vehicle_Headlight_Colors) {
                    if (ImGui::Selectable(color.name.data(), forge.xenonColor == color.value)) {
                        vehicles.QueueForgeCommands({
                            {GTA_Vehicle_Forge_Command_Type::ToggleMod, 22, 1},
                            {GTA_Vehicle_Forge_Command_Type::SetXenonColor, color.value}
                        });
                    }
                }
                ImGui::EndCombo();
            }

            constexpr std::array<const char*, 4> neonSideNames{"Neon Left", "Neon Right", "Neon Front", "Neon Back"};
            for (int side = 0; side < 4; ++side) {
                bool enabled = forge.neonEnabled[static_cast<std::size_t>(side)];
                ImGui::PushID(3000 + side);
                if (ImGui::Checkbox(neonSideNames[static_cast<std::size_t>(side)], &enabled))
                    vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::SetNeonEnabled, side, enabled ? 1 : 0});
                ImGui::PopID();
                if (side != 3) ImGui::SameLine();
            }
            if (ImGui::Button("ALL NEON ON", ImVec2(135.0F, 30.0F)))
                vehicles.QueueForgeCommands({
                    {GTA_Vehicle_Forge_Command_Type::SetNeonEnabled, 0, 1},
                    {GTA_Vehicle_Forge_Command_Type::SetNeonEnabled, 1, 1},
                    {GTA_Vehicle_Forge_Command_Type::SetNeonEnabled, 2, 1},
                    {GTA_Vehicle_Forge_Command_Type::SetNeonEnabled, 3, 1}
                });
            ImGui::SameLine();
            if (ImGui::Button("ALL NEON OFF", ImVec2(135.0F, 30.0F)))
                vehicles.QueueForgeCommands({
                    {GTA_Vehicle_Forge_Command_Type::SetNeonEnabled, 0, 0},
                    {GTA_Vehicle_Forge_Command_Type::SetNeonEnabled, 1, 0},
                    {GTA_Vehicle_Forge_Command_Type::SetNeonEnabled, 2, 0},
                    {GTA_Vehicle_Forge_Command_Type::SetNeonEnabled, 3, 0}
                });

            const GTA_Vehicle_Named_Rgb* currentNeonPreset = nullptr;
            for (const auto& preset : GTA_Vehicle_Neon_Colors)
                if (preset.rgb == forge.neonRgb) currentNeonPreset = &preset;
            ImGui::SetNextItemWidth(280.0F);
            if (ImGui::BeginCombo("Neon Color", currentNeonPreset ? currentNeonPreset->name.data() : "Custom RGB")) {
                for (const auto& preset : GTA_Vehicle_Neon_Colors) {
                    if (ImGui::Selectable(preset.name.data(), currentNeonPreset == &preset))
                        vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::SetNeonColor,
                            preset.rgb[0], preset.rgb[1], preset.rgb[2]});
                }
                ImGui::EndCombo();
            }
            static int neonRgbVehicle = 0;
            static std::array<int, 3> neonRgb{255, 255, 255};
            if (neonRgbVehicle != forge.vehicle) {
                neonRgbVehicle = forge.vehicle;
                neonRgb = forge.neonRgb;
            }
            ImGui::InputInt3("Custom Neon RGB", neonRgb.data());
            if (ImGui::Button("APPLY NEON RGB", ImVec2(165.0F, 30.0F)))
                vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::SetNeonColor, neonRgb[0], neonRgb[1], neonRgb[2]});

            MedievalDivider();
            ImGui::TextColored(Bronze, "TIRES");
            bool tyresCanBurst = forge.tyresCanBurst;
            if (ImGui::Checkbox("Burstable Tires", &tyresCanBurst))
                vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::SetTyresCanBurst, tyresCanBurst ? 1 : 0});
            ImGui::SameLine();
            bool driftTyres = forge.driftTyres;
            if (ImGui::Checkbox("Low Grip Tires", &driftTyres))
                vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::SetDriftTyres, driftTyres ? 1 : 0});

            const GTA_Vehicle_Named_Rgb* currentSmokePreset = nullptr;
            for (const auto& preset : GTA_Vehicle_Tyre_Smoke_Colors)
                if (preset.rgb == forge.tyreSmokeRgb) currentSmokePreset = &preset;
            ImGui::SetNextItemWidth(280.0F);
            if (ImGui::BeginCombo("Tire Smoke Color", currentSmokePreset ? currentSmokePreset->name.data() : "Custom RGB")) {
                for (const auto& preset : GTA_Vehicle_Tyre_Smoke_Colors) {
                    if (ImGui::Selectable(preset.name.data(), currentSmokePreset == &preset)) {
                        vehicles.QueueForgeCommands({
                            {GTA_Vehicle_Forge_Command_Type::ToggleMod, 20, 1},
                            {GTA_Vehicle_Forge_Command_Type::SetTyreSmokeColor, preset.rgb[0], preset.rgb[1], preset.rgb[2]}
                        });
                    }
                }
                ImGui::EndCombo();
            }

            if (ImGui::CollapsingHeader("EXTRAS / LIVERY / INTERIOR", ImGuiTreeNodeFlags_DefaultOpen)) {
                bool anyExtra = false;
                for (int extra = 1; extra <= 14; ++extra) {
                    if (!forge.extraExists[static_cast<std::size_t>(extra)])
                        continue;
                    anyExtra = true;
                    bool enabled = forge.extrasEnabled[static_cast<std::size_t>(extra)];
                    const std::string label = "Extra " + std::to_string(extra);
                    ImGui::PushID(4000 + extra);
                    if (ImGui::Checkbox(label.c_str(), &enabled))
                        vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::SetExtra, extra, enabled ? 1 : 0});
                    ImGui::PopID();
                    if ((extra % 4) != 0) ImGui::SameLine();
                }
                if (!anyExtra) ImGui::TextDisabled("This vehicle reports no toggleable extras.");

                if (forge.liveryCount > 0) {
                    const std::string liveryPreview = forge.livery < 0 ? "Stock" : "Livery " + std::to_string(forge.livery + 1);
                    ImGui::SetNextItemWidth(260.0F);
                    if (ImGui::BeginCombo("Livery", liveryPreview.c_str())) {
                        if (ImGui::Selectable("Stock", forge.livery < 0))
                            vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::SetLivery, -1});
                        for (int index = 0; index < forge.liveryCount; ++index) {
                            const std::string label = "Livery " + std::to_string(index + 1);
                            if (ImGui::Selectable(label.c_str(), forge.livery == index))
                                vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::SetLivery, index});
                        }
                        ImGui::EndCombo();
                    }
                }

                ImGui::SetNextItemWidth(280.0F);
                if (ImGui::BeginCombo("Interior Color", PaintColorPreview(0, forge.interiorColor))) {
                    for (const auto& paint : GTA_Vehicle_Paint_Palette(0))
                        if (ImGui::Selectable(paint.name.data(), forge.interiorColor == paint.value))
                            vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::SetInteriorColor, paint.value});
                    ImGui::EndCombo();
                }
                ImGui::SetNextItemWidth(280.0F);
                if (ImGui::BeginCombo("Dashboard Color", PaintColorPreview(0, forge.dashboardColor))) {
                    for (const auto& paint : GTA_Vehicle_Paint_Palette(0))
                        if (ImGui::Selectable(paint.name.data(), forge.dashboardColor == paint.value))
                            vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::SetDashboardColor, paint.value});
                    ImGui::EndCombo();
                }
            }

            MedievalDivider();
            ImGui::TextColored(Bronze, "WHEELS");
            ImGui::SetNextItemWidth(260.0F);
            const char* wheelPreview = LookupVehicleValue(GTA_Vehicle_Wheel_Types, m_forgeWheelType);
            if (ImGui::BeginCombo("Wheel Category", wheelPreview)) {
                for (const auto& wheelType : GTA_Vehicle_Wheel_Types) {
                    if (ImGui::Selectable(wheelType.name.data(), m_forgeWheelType == wheelType.value)) {
                        m_forgeWheelType = wheelType.value;
                        m_forgeWheelIndex = -1;
                        vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::SetWheelType, wheelType.value});
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();
            ImGui::Checkbox("Custom Tires", &m_forgeCustomTires);

            const auto frontWheels = std::find_if(forge.categories.begin(), forge.categories.end(),
                [](const GTA_Vehicle_Forge_Category& category) { return category.slot == 23; });
            if (forge.wheelType != m_forgeWheelType) {
                ImGui::TextDisabled("Refreshing named rims for %s...", wheelPreview);
            } else if (frontWheels == forge.categories.end()) {
                ImGui::TextDisabled("This vehicle does not expose front-wheel modifications for this category.");
            } else if (ImGui::BeginChild("##ForgeRims", ImVec2(0.0F, 175.0F), ImGuiChildFlags_Borders)) {
                ImGui::TextColored(Bronze, "%s RIMS", wheelPreview);
                for (const auto& option : frontWheels->options) {
                    const bool installed = option.index == frontWheels->installedIndex;
                    ImGui::PushID(option.index);
                    if (ImGui::Selectable(option.name.c_str(), installed)) {
                        m_forgeWheelIndex = option.index;
                        vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::SetMod,
                            23, option.index, m_forgeCustomTires ? 1 : 0});
                    }
                    ImGui::PopID();
                }
                ImGui::EndChild();
            } else {
                ImGui::EndChild();
            }

            MedievalDivider();
            ImGui::TextColored(Bronze, "WINDOWS / PLATES");
            ImGui::SetNextItemWidth(260.0F);
            if (ImGui::BeginCombo("Window Tint", LookupVehicleValue(GTA_Vehicle_Window_Tints, m_forgeWindowTint))) {
                for (const auto& tint : GTA_Vehicle_Window_Tints) {
                    if (ImGui::Selectable(tint.name.data(), m_forgeWindowTint == tint.value)) {
                        m_forgeWindowTint = tint.value;
                        vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::SetWindowTint, tint.value});
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::SetNextItemWidth(260.0F);
            if (ImGui::BeginCombo("Plate Style", LookupVehicleValue(GTA_Vehicle_Plate_Styles, m_forgePlateStyle))) {
                for (const auto& plate : GTA_Vehicle_Plate_Styles) {
                    if (ImGui::Selectable(plate.name.data(), m_forgePlateStyle == plate.value)) {
                        m_forgePlateStyle = plate.value;
                        vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::SetPlateStyle, plate.value});
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::SetNextItemWidth(260.0F);
            ImGui::InputText("Plate Text", m_forgePlateText.data(), m_forgePlateText.size());
            ImGui::SameLine();
            if (ImGui::Button("APPLY PLATE TEXT", ImVec2(165.0F, 32.0F)))
                vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::SetPlateText, 0, 0, 0, m_forgePlateText.data()});

            MedievalDivider();
            ImGui::TextColored(Bronze, "PAINT / FINISH");
            ImGui::TextDisabled("Named LSC-style palettes. The current vehicle state is read back on the game thread, including LSC changes.");

            auto drawPaintSelector = [&](const char* idPrefix, const char* label,
                int& paintType, int& color, bool primary) {
                ImGui::PushID(idPrefix);
                ImGui::SetNextItemWidth(220.0F);
                if (ImGui::BeginCombo("Paint Type", LookupVehicleValue(GTA_Vehicle_Paint_Types, paintType))) {
                    for (const auto& type : GTA_Vehicle_Paint_Types) {
                        if (ImGui::Selectable(type.name.data(), paintType == type.value)) {
                            paintType = type.value;
                            if (!PaintPaletteContains(paintType, color))
                                color = FirstPaintColor(paintType);
                            if (primary) {
                                vehicles.QueueForgeCommands({
                                    {GTA_Vehicle_Forge_Command_Type::ClearCustomPrimary},
                                    {GTA_Vehicle_Forge_Command_Type::SetPrimaryPaint, paintType, color, m_forgePearlescent}
                                });
                            } else {
                                vehicles.QueueForgeCommands({
                                    {GTA_Vehicle_Forge_Command_Type::ClearCustomSecondary},
                                    {GTA_Vehicle_Forge_Command_Type::SetSecondaryPaint, paintType, color, 0}
                                });
                            }
                        }
                    }
                    ImGui::EndCombo();
                }
                ImGui::SameLine();
                ImGui::TextColored(Bronze, "%s", label);

                ImGui::SetNextItemWidth(320.0F);
                if (ImGui::BeginCombo("Color", PaintColorPreview(paintType, color))) {
                    for (const auto& paint : GTA_Vehicle_Paint_Palette(paintType)) {
                        if (ImGui::Selectable(paint.name.data(), color == paint.value)) {
                            color = paint.value;
                            if (primary) {
                                vehicles.QueueForgeCommands({