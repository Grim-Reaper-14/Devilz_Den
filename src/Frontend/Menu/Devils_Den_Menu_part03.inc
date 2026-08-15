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
                ImGui::PopID();
            };

            drawPaintSelector("PrimaryPaint", "PRIMARY", m_forgePrimaryPaintType, m_forgePrimaryColor, true);
            drawPaintSelector("SecondaryPaint", "SECONDARY", m_forgeSecondaryPaintType, m_forgeSecondaryColor, false);

            ImGui::SetNextItemWidth(320.0F);
            if (ImGui::BeginCombo("Pearlescent", PaintColorPreview(0, m_forgePearlescent))) {
                for (const auto& paint : GTA_Vehicle_Paint_Palette(0)) {
                    if (ImGui::Selectable(paint.name.data(), m_forgePearlescent == paint.value)) {
                        m_forgePearlescent = paint.value;
                        vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::SetExtraColours,
                            m_forgePearlescent, m_forgeWheelColor});
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::SetNextItemWidth(320.0F);
            if (ImGui::BeginCombo("Wheel Color", PaintColorPreview(0, m_forgeWheelColor))) {
                for (const auto& paint : GTA_Vehicle_Paint_Palette(0)) {
                    if (ImGui::Selectable(paint.name.data(), m_forgeWheelColor == paint.value)) {
                        m_forgeWheelColor = paint.value;
                        vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::SetExtraColours,
                            m_forgePearlescent, m_forgeWheelColor});
                    }
                }
                ImGui::EndCombo();
            }

            if (ImGui::CollapsingHeader("CUSTOM RGB")) {
                ImGui::InputInt3("Primary RGB", m_forgePrimaryRgb.data());
                if (ImGui::Button("APPLY PRIMARY RGB", ImVec2(180.0F, 34.0F)))
                    vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::SetCustomPrimaryRgb,
                        m_forgePrimaryRgb[0], m_forgePrimaryRgb[1], m_forgePrimaryRgb[2]});
                ImGui::SameLine();
                ImGui::TextDisabled("Current: %s", forge.primaryCustom ? "CUSTOM" : "PALETTE");

                ImGui::InputInt3("Secondary RGB", m_forgeSecondaryRgb.data());
                if (ImGui::Button("APPLY SECONDARY RGB", ImVec2(190.0F, 34.0F)))
                    vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::SetCustomSecondaryRgb,
                        m_forgeSecondaryRgb[0], m_forgeSecondaryRgb[1], m_forgeSecondaryRgb[2]});
                ImGui::SameLine();
                ImGui::TextDisabled("Current: %s", forge.secondaryCustom ? "CUSTOM" : "PALETTE");
            }

            if (ImGui::CollapsingHeader("ADVANCED / RAW PAINT INDEXES")) {
                ImGui::TextDisabled("Debug fallback only. Named palettes above are the normal workflow.");
                ImGui::InputInt("Primary Paint Type##Raw", &m_forgePrimaryPaintType);
                ImGui::InputInt("Primary Color Index##Raw", &m_forgePrimaryColor);
                ImGui::InputInt("Secondary Paint Type##Raw", &m_forgeSecondaryPaintType);
                ImGui::InputInt("Secondary Color Index##Raw", &m_forgeSecondaryColor);
                ImGui::InputInt("Pearlescent Index##Raw", &m_forgePearlescent);
                ImGui::InputInt("Wheel Color Index##Raw", &m_forgeWheelColor);
                if (ImGui::Button("APPLY RAW PAINT", ImVec2(170.0F, 34.0F))) {
                    vehicles.QueueForgeCommands({
                        {GTA_Vehicle_Forge_Command_Type::ClearCustomPrimary},
                        {GTA_Vehicle_Forge_Command_Type::ClearCustomSecondary},
                        {GTA_Vehicle_Forge_Command_Type::SetPrimaryPaint, m_forgePrimaryPaintType, m_forgePrimaryColor, m_forgePearlescent},
                        {GTA_Vehicle_Forge_Command_Type::SetSecondaryPaint, m_forgeSecondaryPaintType, m_forgeSecondaryColor, 0},
                        {GTA_Vehicle_Forge_Command_Type::SetExtraColours, m_forgePearlescent, m_forgeWheelColor, 0}
                    });
                }
            }
        }
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("SAVED BUILDS")) {
        ImGui::TextColored(Bronze, "SAVED VEHICLE BUILDS");
        ImGui::SameLine();
        if (ImGui::Button("RELOAD", ImVec2(90.0F, 28.0F))) {
            reloadSavedBuilds();
            savedBuildStatus = "Reloaded saved vehicle JSON files.";
        }
        ImGui::TextDisabled("Folder: %s", GTA_Vehicle_Saved_Builds::RootDirectory().string().c_str());

        if (forge.vehicle != 0) {
            ImGui::SetNextItemWidth(300.0F);
            ImGui::InputTextWithHint("##SavedBuildName", "Build name...", savedBuildName.data(), savedBuildName.size());
            ImGui::SameLine();
            if (ImGui::Button("SAVE BUILD TO JSON", ImVec2(205.0F, 34.0F))) {
                std::string error;
                auto build = GTA_Vehicle_Saved_Builds::Capture(savedBuildName.data(), currentMetadata, forge);
                if (GTA_Vehicle_Saved_Builds::Save(build, &error)) {
                    savedBuildStatus = "Saved actual current vehicle state to JSON.";
                    reloadSavedBuilds();
                } else {
                    savedBuildStatus = error.empty() ? "Saving vehicle failed." : error;
                }
            }
            ImGui::SameLine();
            ImGui::BeginDisabled(spawnBusy);
            if (ImGui::Button("CLONE CURRENT", ImVec2(145.0F, 34.0F))) {
                auto clone = GTA_Vehicle_Saved_Builds::Capture("Current Clone", currentMetadata, forge);
                GTA_Vehicle_Spawn_Options options{};
                options.spawnInside = false;
                options.placeOnGround = true;
                options.engineRunning = true;
                options.clean = true;
                options.invincible = vehicles.VehicleGodMode();
                vehicles.SetPostSpawnForgeCommands(GTA_Vehicle_Saved_Builds::BuildCommands(clone));
                vehicles.RequestSpawn(clone.modelHash, options);
                savedBuildStatus = "Cloning current vehicle beside the player.";
            }
            ImGui::EndDisabled();
        } else {
            ImGui::TextDisabled("Enter a vehicle to save or clone its current build.");
        }

        if (!savedBuildStatus.empty()) ImGui::TextWrapped("%s", savedBuildStatus.c_str());
        MedievalDivider();

        if (ImGui::BeginChild("##SavedBuildList", ImVec2(300.0F, 320.0F), ImGuiChildFlags_Borders)) {
            ImGui::TextColored(Bronze, "BUILDS (%zu)", savedBuilds.size());
            for (std::size_t index = 0; index < savedBuilds.size(); ++index) {
                ImGui::PushID(static_cast<int>(index));
                if (ImGui::Selectable(savedBuilds[index].name.c_str(), selectedSavedBuild == static_cast<int>(index)))
                    selectedSavedBuild = static_cast<int>(index);
                ImGui::PopID();
            }
            if (savedBuilds.empty()) ImGui::TextDisabled("No saved vehicle JSON files yet.");
        }
        ImGui::EndChild();
        ImGui::SameLine();
        if (ImGui::BeginChild("##SavedBuildDetails", ImVec2(0.0F, 320.0F), ImGuiChildFlags_Borders)) {
            if (selectedSavedBuild >= 0 && selectedSavedBuild < static_cast<int>(savedBuilds.size())) {
                const auto& build = savedBuilds[static_cast<std::size_t>(selectedSavedBuild)];
                ImGui::TextColored(Bronze, "%s", build.name.c_str());
                ImGui::Text("%s %s", build.makeName.c_str(), build.displayName.c_str());
                ImGui::TextDisabled("Model: %s | 0x%08X", build.modelName.c_str(), build.modelHash);
                ImGui::Separator();
                ImGui::Text("Wheels: %s", build.wheelTypeName.empty() ? "Unknown" : build.wheelTypeName.c_str());
                ImGui::Text("Primary: %s / %s", build.primaryPaintTypeName.c_str(), build.primaryColorName.c_str());
                ImGui::Text("Secondary: %s / %s", build.secondaryPaintTypeName.c_str(), build.secondaryColorName.c_str());
                ImGui::Text("Pearl: %s", build.pearlescentColorName.c_str());
                ImGui::Text("Plate: %s  [%s]", build.plateText.c_str(), build.plateStyleName.c_str());
                ImGui::Text("Mods captured: %zu", build.mods.size());

                ImGui::BeginDisabled(spawnBusy);
                if (ImGui::Button("SPAWN BUILD", ImVec2(140.0F, 36.0F))) {
                    GTA_Vehicle_Spawn_Options options{};
                    options.spawnInside = true;
                    options.placeOnGround = true;
                    options.engineRunning = true;
                    options.clean = true;
                    options.invincible = vehicles.VehicleGodMode();
                    vehicles.SetPostSpawnForgeCommands(GTA_Vehicle_Saved_Builds::BuildCommands(build));
                    vehicles.RequestSpawn(build.modelHash, options);
                    savedBuildStatus = "Spawning saved JSON build.";
                }
                ImGui::EndDisabled();
                ImGui::SameLine();
                ImGui::BeginDisabled(forge.vehicle == 0);
                if (ImGui::Button("APPLY TO CURRENT", ImVec2(155.0F, 36.0F))) {
                    GTA_Vehicle_Saved_Builds::QueueApply(build, vehicles);
                    savedBuildStatus = "Applying saved build to current vehicle.";
                }
                ImGui::EndDisabled();
                ImGui::SameLine();
                if (ImGui::Button("DELETE", ImVec2(95.0F, 36.0F))) {
                    std::string error;
                    if (GTA_Vehicle_Saved_Builds::Remove(build, &error)) {
                        savedBuildStatus = "Deleted saved vehicle JSON.";
                        reloadSavedBuilds();
                    } else {
                        savedBuildStatus = error.empty() ? "Delete failed." : error;
                    }
                }
            } else {
                ImGui::TextDisabled("Select a saved build to inspect, spawn, apply, or delete it.");
            }
        }
        ImGui::EndChild();
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("HANDLING")) {
        ImGui::TextColored(Bronze, "HANDLING / SUSPENSION");
        if (forge.vehicle == 0) {
            ImGui::TextDisabled("Enter a vehicle to edit handling-related options.");
        } else {
            const auto suspension = std::find_if(forge.categories.begin(), forge.categories.end(),
                [](const GTA_Vehicle_Forge_Category& category) { return category.slot == 15; });
            if (suspension != forge.categories.end()) {
                static std::uint32_t suspensionModel = 0;
                static int suspensionIndex = -1;
                if (suspensionModel != forge.modelHash) {
                    suspensionModel = forge.modelHash;
                    suspensionIndex = suspension->installedIndex;
                } else {
                    suspensionIndex = suspension->installedIndex;
                }
                const int maxIndex = static_cast<int>(suspension->options.size()) - 2;
                ImGui::SetNextItemWidth(520.0F);
                if (ImGui::SliderInt("LSC Suspension Level", &suspensionIndex, -1, (std::max)(maxIndex, -1)))
                    vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::SetMod, 15, suspensionIndex, 0});

                const auto option = std::find_if(suspension->options.begin(), suspension->options.end(),
                    [](const GTA_Vehicle_Forge_Option& value) { return value.index == suspensionIndex; });
                if (option != suspension->options.end()) ImGui::Text("Selected: %s", option->name.c_str());
                ImGui::TextDisabled("Move the slider live: -1 is stock; higher supported levels apply immediately.");
            } else {
                ImGui::TextDisabled("This vehicle does not expose an LSC suspension mod slot.");
            }

            bool reducedForce = m_forgeLoweredStance;
            if (ImGui::Checkbox("Reduced Suspension Force", &reducedForce)) {
                m_forgeLoweredStance = reducedForce;
                vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::SetLoweredStance, reducedForce ? 1 : 0});
            }
            if (ImGui::Button("STOCK SUSPENSION", ImVec2(170.0F, 36.0F))) {
                m_forgeLoweredStance = false;
                vehicles.QueueForgeCommands({
                    {GTA_Vehicle_Forge_Command_Type::SetMod, 15, -1, 0},
                    {GTA_Vehicle_Forge_Command_Type::SetLoweredStance, 0}
                });
            }
            if (suspension != forge.categories.end() && suspension->options.size() > 1U) {
                ImGui::SameLine();
                if (ImGui::Button("LOWEST LSC SETTING", ImVec2(190.0F, 36.0F))) {
                    const int lowest = static_cast<int>(suspension->options.size()) - 2;
                    vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::SetMod, 15, lowest, 0});
                }
            }

            MedievalDivider();
            ImGui::TextColored(Bronze, "VEHICLE PROTECTION");
            bool keepPerfect = vehicles.KeepVehiclePerfect();
            if (ImGui::Checkbox("Always Keep Vehicle Perfect##Handling", &keepPerfect)) vehicles.SetKeepVehiclePerfect(keepPerfect);
            bool vehicleGodMode = vehicles.VehicleGodMode();
            if (ImGui::Checkbox("Vehicle God Mode##Handling", &vehicleGodMode)) vehicles.SetVehicleGodMode(vehicleGodMode);

            ImGui::Spacing();
            ImGui::TextWrapped("Deep Yim-style handling floats such as true ride height, traction, drive force, braking and steering are still locked until the exact GTA V Enhanced handling layout is validated. This tab deliberately stays native-backed until then.");
        }
        ImGui::EndTabItem();
    }
