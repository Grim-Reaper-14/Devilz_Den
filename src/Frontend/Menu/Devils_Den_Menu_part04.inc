    ImGui::EndTabBar();
}

void Devils_Den_Menu::DrawTeleportPage()
{
    auto& gameplay = GTA_Gameplay_State::Instance();
    ImGui::TextColored(EmberRed, "TELEPORT");
    ImGui::SameLine();
    ImGui::TextDisabled("- waypoint and landmark travel");
    MedievalDivider();
    ImGui::TextColored(Bronze, "WAYPOINT");
    ImGui::TextWrapped("Place a waypoint on the Story Mode map, then use the button below. If you are actually seated in a vehicle, the vehicle teleports with you.");

    const auto status = gameplay.TeleportStatus();
    const bool busy = status == GTA_Teleport_Waypoint_Status::Queued || status == GTA_Teleport_Waypoint_Status::Resolving;
    ImGui::BeginDisabled(busy);
    if (ImGui::Button("TELEPORT TO WAYPOINT", ImVec2(240.0F, 44.0F))) gameplay.RequestTeleportToWaypoint();
    ImGui::EndDisabled();
    ImGui::TextColored(Bronze, "STATUS");
    ImGui::SameLine();
    ImGui::TextUnformatted(TeleportStatusText(status));
    MedievalDivider();
    ImGui::TextColored(Bronze, "PLACES");

    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float buttonWidth = (ImGui::GetContentRegionAvail().x - spacing) * 0.5F;
    ImGui::BeginDisabled(busy);
    for (std::size_t index = 0; index < GTA_Teleport_Locations.size(); ++index) {
        const auto& location = GTA_Teleport_Locations[index];
        ImGui::PushID(static_cast<int>(index));
        if (ImGui::Button(location.label.data(), ImVec2(buttonWidth, 34.0F))) gameplay.RequestTeleportToLocation(location.id);
        ImGui::PopID();
        if ((index % 2U) == 0U) ImGui::SameLine();
    }
    ImGui::EndDisabled();
    ImGui::TextDisabled("Waypoint travel resolves ground, water, then approximate terrain. On foot, only the player moves.");
}

void Devils_Den_Menu::DrawSettingsPage()
{
    auto& gameplay = GTA_Gameplay_State::Instance();
    auto& vehicles = GTA_Vehicle_State::Instance();

    static bool configsLoaded = false;
    static std::vector<Devils_Den_Config> configs;
    static int selectedConfig = -1;
    static std::array<char, 64> configName{};
    static bool seededName = false;
    static std::string configStatus;

    if (!seededName) {
        CopyToBuffer("Default", configName.data(), configName.size());
        seededName = true;
    }

    const auto reloadConfigs = [&]() {
        configs = Devils_Den_Config_Store::LoadAll();
        if (configs.empty()) selectedConfig = -1;
        else selectedConfig = std::clamp(selectedConfig, 0, static_cast<int>(configs.size()) - 1);
        configsLoaded = true;
    };
    if (!configsLoaded) reloadConfigs();

    ImGui::TextColored(EmberRed, "SETTINGS");
    ImGui::SameLine();
    ImGui::TextDisabled("- configuration and persistence");
    MedievalDivider();

    ImGui::TextColored(Bronze, "CONFIGS");
    ImGui::TextDisabled("Folder: %s", Devils_Den_Config_Store::RootDirectory().string().c_str());
    ImGui::SetNextItemWidth(280.0F);
    ImGui::InputTextWithHint("##ConfigName", "Config name...", configName.data(), configName.size());
    ImGui::SameLine();
    if (ImGui::Button("SAVE CONFIG", ImVec2(130.0F, 34.0F))) {
        Devils_Den_Config config{};
        config.name = configName.data();
        config.godMode = gameplay.GodMode();
        config.neverWanted = gameplay.NeverWanted();
        config.superJump = gameplay.SuperJump();
        config.infiniteOxygen = gameplay.InfiniteOxygen();
        config.noRagdoll = gameplay.NoRagdoll();
        config.keepPlayerClean = gameplay.KeepPlayerClean();
        config.infiniteAmmo = gameplay.InfiniteAmmo();
        config.unlimitedClip = gameplay.UnlimitedClip();
        config.explosiveBullets = gameplay.ExplosiveBullets();
        config.explosionType = gameplay.ExplosionType();
        config.explosionDamageScale = gameplay.ExplosionDamageScale();
        config.explosionCameraShake = gameplay.ExplosionCameraShake();
        config.keepVehiclePerfect = vehicles.KeepVehiclePerfect();
        config.vehicleGodMode = vehicles.VehicleGodMode();
        config.spawnInsideVehicle = m_spawnInsideVehicle;
        config.spawnVehicleMaxed = m_spawnVehicleMaxed;
        config.spawnVehicleOnGround = m_spawnVehicleOnGround;
        config.spawnVehicleEngineRunning = m_spawnVehicleEngineRunning;
        config.spawnVehicleInvincible = m_spawnVehicleInvincible;
        config.spawnVehicleClean = m_spawnVehicleClean;

        std::string error;
        if (Devils_Den_Config_Store::Save(config, &error)) {
            configStatus = "Saved config JSON.";
            reloadConfigs();
        } else {
            configStatus = error.empty() ? "Saving config failed." : error;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("RELOAD", ImVec2(95.0F, 34.0F))) {
        reloadConfigs();
        configStatus = "Reloaded config JSON files.";
    }
    if (!configStatus.empty()) ImGui::TextWrapped("%s", configStatus.c_str());

    MedievalDivider();
    if (ImGui::BeginChild("##ConfigList", ImVec2(280.0F, 300.0F), ImGuiChildFlags_Borders)) {
        ImGui::TextColored(Bronze, "SAVED CONFIGS (%zu)", configs.size());
        for (std::size_t index = 0; index < configs.size(); ++index) {
            ImGui::PushID(static_cast<int>(index));
            if (ImGui::Selectable(configs[index].name.c_str(), selectedConfig == static_cast<int>(index))) {
                selectedConfig = static_cast<int>(index);
                CopyToBuffer(configs[index].name, configName.data(), configName.size());
            }
            ImGui::PopID();
        }
        if (configs.empty()) ImGui::TextDisabled("No configs saved yet.");
    }
    ImGui::EndChild();
    ImGui::SameLine();

    if (ImGui::BeginChild("##ConfigDetails", ImVec2(0.0F, 300.0F), ImGuiChildFlags_Borders)) {
        if (selectedConfig >= 0 && selectedConfig < static_cast<int>(configs.size())) {
            const auto& config = configs[static_cast<std::size_t>(selectedConfig)];
            ImGui::TextColored(Bronze, "%s", config.name.c_str());
            ImGui::Text("Infinite Ammo: %s", config.infiniteAmmo ? "ON" : "OFF");
            ImGui::Text("Unlimited Clip: %s", config.unlimitedClip ? "ON" : "OFF");
            ImGui::Text("Explosive Bullets: %s", config.explosiveBullets ? "ON" : "OFF");
            ImGui::Text("Explosion: %s", LookupNamedValue(ExplosionTypes, config.explosionType, "Unknown"));
            ImGui::Text("Keep Vehicle Perfect: %s", config.keepVehiclePerfect ? "ON" : "OFF");
            ImGui::Text("Vehicle God Mode: %s", config.vehicleGodMode ? "ON" : "OFF");
            ImGui::Separator();

            if (ImGui::Button("LOAD CONFIG", ImVec2(140.0F, 36.0F))) {
                gameplay.SetGodMode(config.godMode);
                gameplay.SetNeverWanted(config.neverWanted);
                gameplay.SetSuperJump(config.superJump);
                gameplay.SetInfiniteOxygen(config.infiniteOxygen);
                gameplay.SetNoRagdoll(config.noRagdoll);
                gameplay.SetKeepPlayerClean(config.keepPlayerClean);
                gameplay.SetInfiniteAmmo(config.infiniteAmmo);
                gameplay.SetUnlimitedClip(config.unlimitedClip);
                gameplay.SetExplosiveBullets(config.explosiveBullets);
                gameplay.SetExplosionType(config.explosionType);
                gameplay.SetExplosionDamageScale(config.explosionDamageScale);
                gameplay.SetExplosionCameraShake(config.explosionCameraShake);
                vehicles.SetKeepVehiclePerfect(config.keepVehiclePerfect);
                vehicles.SetVehicleGodMode(config.vehicleGodMode);
                m_spawnInsideVehicle = config.spawnInsideVehicle;
                m_spawnVehicleMaxed = config.spawnVehicleMaxed;
                m_spawnVehicleOnGround = config.spawnVehicleOnGround;
                m_spawnVehicleEngineRunning = config.spawnVehicleEngineRunning;
                m_spawnVehicleInvincible = config.spawnVehicleInvincible;
                m_spawnVehicleClean = config.spawnVehicleClean;
                configStatus = "Loaded config and applied live settings.";
            }
            ImGui::SameLine();
            if (ImGui::Button("DELETE CONFIG", ImVec2(145.0F, 36.0F))) {
                std::string error;
                if (Devils_Den_Config_Store::Remove(config, &error)) {
                    configStatus = "Deleted config JSON.";
                    reloadConfigs();
                } else {
                    configStatus = error.empty() ? "Delete failed." : error;
                }
            }
        } else {
            ImGui::TextDisabled("Select a config to inspect, load, or delete it.");
        }
    }
    ImGui::EndChild();
    ImGui::TextDisabled("Configs persist live Self/Weapons settings, vehicle protection, and vehicle-spawn defaults.");
}

void Devils_Den_Menu::DrawPlaceholderPage(const char* title, const char* detail)
{
    ImGui::TextColored(EmberRed, "%s", title);
    MedievalDivider();
    ImGui::TextWrapped("%s", detail);
    ImGui::Spacing();
    ImGui::TextDisabled("This page will use the same Devil's Den medieval frame and control language.");
}
}
