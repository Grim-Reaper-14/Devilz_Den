            selected ? ImVec4(0.52F, 0.075F, 0.045F, 1.00F) : ImVec4(0.24F, 0.055F, 0.040F, 1.00F));
        if (ImGui::Button(entry.label, ImVec2(-1.0F, 42.0F)))
            m_page = entry.page;
        ImGui::PopStyleColor(2);
    }

    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 48.0F);
    ImGui::TextDisabled("INSERT  -  toggle menu");
    ImGui::PopStyleColor();
}

void Devils_Den_Menu::DrawSelfPage()
{
    auto& gameplay = GTA_Gameplay_State::Instance();
    ImGui::TextColored(EmberRed, "SELF");
    ImGui::SameLine();
    ImGui::TextDisabled("- live Story Mode features");
    MedievalDivider();
    ImGui::TextColored(Bronze, "PLAYER OPTIONS");

    m_godMode = gameplay.GodMode();
    if (ImGui::Checkbox("God Mode", &m_godMode)) gameplay.SetGodMode(m_godMode);
    m_neverWanted = gameplay.NeverWanted();
    if (ImGui::Checkbox("Never Wanted", &m_neverWanted)) gameplay.SetNeverWanted(m_neverWanted);
    m_superJump = gameplay.SuperJump();
    if (ImGui::Checkbox("Super Jump", &m_superJump)) gameplay.SetSuperJump(m_superJump);
    m_infiniteOxygen = gameplay.InfiniteOxygen();
    if (ImGui::Checkbox("Infinite Oxygen", &m_infiniteOxygen)) gameplay.SetInfiniteOxygen(m_infiniteOxygen);
    m_noRagdoll = gameplay.NoRagdoll();
    if (ImGui::Checkbox("No Ragdoll", &m_noRagdoll)) gameplay.SetNoRagdoll(m_noRagdoll);
    m_keepPlayerClean = gameplay.KeepPlayerClean();
    if (ImGui::Checkbox("Keep Player Clean", &m_keepPlayerClean)) gameplay.SetKeepPlayerClean(m_keepPlayerClean);

    ImGui::BeginDisabled();
    ImGui::Checkbox("Fast Run", &m_fastRun);
    ImGui::SliderFloat("Health", &m_health, 0.0F, 100.0F, "%.0f");
    ImGui::EndDisabled();
    MedievalDivider();
    ImGui::TextDisabled("Weapon controls live on the WEAPONS page. Live features execute on the validated game thread.");
}

void Devils_Den_Menu::DrawWeaponsPage()
{
    auto& gameplay = GTA_Gameplay_State::Instance();
    ImGui::TextColored(EmberRed, "WEAPONS");
    ImGui::SameLine();
    ImGui::TextDisabled("- Story Mode armory and bullet effects");
    MedievalDivider();

    ImGui::TextColored(Bronze, "AMMO");
    m_infiniteAmmo = gameplay.InfiniteAmmo();
    if (ImGui::Checkbox("Infinite Ammo", &m_infiniteAmmo)) gameplay.SetInfiniteAmmo(m_infiniteAmmo);
    bool unlimitedClip = gameplay.UnlimitedClip();
    if (ImGui::Checkbox("Unlimited Clip / No Reload", &unlimitedClip)) gameplay.SetUnlimitedClip(unlimitedClip);

    MedievalDivider();
    ImGui::TextColored(Bronze, "LOADOUT");
    if (ImGui::Button("GIVE ALL WEAPONS", ImVec2(180.0F, 38.0F))) gameplay.RequestGiveAllWeapons();
    ImGui::SameLine();
    if (ImGui::Button("GIVE MAX AMMO", ImVec2(170.0F, 38.0F))) gameplay.RequestGiveMaxAmmo();
    ImGui::SameLine();
    if (ImGui::Button("REMOVE ALL WEAPONS", ImVec2(190.0F, 38.0F))) gameplay.RequestRemoveAllWeapons();

    MedievalDivider();
    ImGui::TextColored(Bronze, "BULLET EFFECTS");
    bool explosiveBullets = gameplay.ExplosiveBullets();
    if (ImGui::Checkbox("Explosive Bullets", &explosiveBullets)) gameplay.SetExplosiveBullets(explosiveBullets);

    int explosionType = gameplay.ExplosionType();
    ImGui::SetNextItemWidth(300.0F);
    if (ImGui::BeginCombo("Explosion Type", LookupNamedValue(ExplosionTypes, explosionType, "Unknown"))) {
        for (const auto& type : ExplosionTypes) {
            if (ImGui::Selectable(type.name, explosionType == type.value)) {
                explosionType = type.value;
                gameplay.SetExplosionType(type.value);
            }
        }
        ImGui::EndCombo();
    }

    float damageScale = gameplay.ExplosionDamageScale();
    ImGui::SetNextItemWidth(430.0F);
    if (ImGui::SliderFloat("Explosion Damage Scale", &damageScale, 0.0F, 1000.0F, "%.1f"))
        gameplay.SetExplosionDamageScale(damageScale);

    float cameraShake = gameplay.ExplosionCameraShake();
    ImGui::SetNextItemWidth(430.0F);
    if (ImGui::SliderFloat("Explosion Camera Shake", &cameraShake, 0.0F, 10.0F, "%.2f"))
        gameplay.SetExplosionCameraShake(cameraShake);
}

void Devils_Den_Menu::DrawVehiclePage()
{
    auto& vehicles = GTA_Vehicle_State::Instance();

    static std::uint64_t cachedGeneration = static_cast<std::uint64_t>(-1);
    static std::vector<GTA_Vehicle_Metadata> catalog;
    if (cachedGeneration != vehicles.CatalogGeneration()) {
        catalog = vehicles.CatalogSnapshot();
        cachedGeneration = vehicles.CatalogGeneration();
    }

    static std::uint64_t cachedForgeGeneration = static_cast<std::uint64_t>(-1);
    static GTA_Vehicle_Forge_Snapshot forge;
    if (cachedForgeGeneration != vehicles.ForgeSnapshotGeneration()) {
        forge = vehicles.ForgeSnapshot();
        cachedForgeGeneration = vehicles.ForgeSnapshotGeneration();
        if (forge.wheelType >= 0) m_forgeWheelType = forge.wheelType;
        if (forge.windowTint >= 0) m_forgeWindowTint = forge.windowTint;
        if (forge.plateStyle >= 0) m_forgePlateStyle = forge.plateStyle;
        if (!forge.plateText.empty())
            CopyToBuffer(forge.plateText, m_forgePlateText.data(), m_forgePlateText.size());

        if (forge.paintStateReady) {
            if (forge.primaryPaintType >= 0) m_forgePrimaryPaintType = forge.primaryPaintType;
            if (forge.primaryColor >= 0) m_forgePrimaryColor = forge.primaryColor;
            if (forge.secondaryPaintType >= 0) m_forgeSecondaryPaintType = forge.secondaryPaintType;
            if (forge.secondaryColor >= 0) m_forgeSecondaryColor = forge.secondaryColor;
            if (forge.pearlescentColor >= 0) m_forgePearlescent = forge.pearlescentColor;
            if (forge.wheelColor >= 0) m_forgeWheelColor = forge.wheelColor;
            m_forgePrimaryRgb = forge.primaryRgb;
            m_forgeSecondaryRgb = forge.secondaryRgb;
        }

        const auto selectedCategory = std::find_if(forge.categories.begin(), forge.categories.end(),
            [this](const GTA_Vehicle_Forge_Category& category) { return category.slot == m_forgeModSlot; });
        if (selectedCategory != forge.categories.end())
            m_forgeModIndex = selectedCategory->installedIndex;
        else {
            const auto firstCategory = std::find_if(forge.categories.begin(), forge.categories.end(),
                [](const GTA_Vehicle_Forge_Category& category) { return category.slot != 23 && category.slot != 24; });
            if (firstCategory != forge.categories.end()) {
                m_forgeModSlot = firstCategory->slot;
                m_forgeModIndex = firstCategory->installedIndex;
            }
        }

        const auto frontWheels = std::find_if(forge.categories.begin(), forge.categories.end(),
            [](const GTA_Vehicle_Forge_Category& category) { return category.slot == 23; });
        if (frontWheels != forge.categories.end()) {
            m_forgeWheelIndex = frontWheels->installedIndex;
            m_forgeCustomTires = forge.frontCustomTires;
        }
    }

    const GTA_Vehicle_Metadata* currentMetadata = nullptr;
    for (const auto& entry : catalog) {
        if (entry.modelHash == forge.modelHash) {
            currentMetadata = &entry;
            break;
        }
    }

    const auto spawnStatus = vehicles.SpawnStatus();
    const bool spawnBusy = spawnStatus == GTA_Vehicle_Spawn_Status::Queued ||
        spawnStatus == GTA_Vehicle_Spawn_Status::Validating ||
        spawnStatus == GTA_Vehicle_Spawn_Status::Streaming ||
        spawnStatus == GTA_Vehicle_Spawn_Status::Creating ||
        spawnStatus == GTA_Vehicle_Spawn_Status::Applying;

    static bool savedBuildsLoaded = false;
    static std::vector<GTA_Vehicle_Saved_Build> savedBuilds;
    static int selectedSavedBuild = -1;
    static std::array<char, 64> savedBuildName{};
    static std::uint32_t savedBuildNameModel = 0;
    static std::string savedBuildStatus;

    const auto reloadSavedBuilds = [&]() {
        savedBuilds = GTA_Vehicle_Saved_Builds::LoadAll();
        if (savedBuilds.empty())
            selectedSavedBuild = -1;
        else
            selectedSavedBuild = std::clamp(selectedSavedBuild, 0, static_cast<int>(savedBuilds.size()) - 1);
        savedBuildsLoaded = true;
    };
    if (!savedBuildsLoaded)
        reloadSavedBuilds();

    if (forge.modelHash != 0 && forge.modelHash != savedBuildNameModel) {
        const std::string defaultName = currentMetadata && !currentMetadata->displayName.empty()
            ? currentMetadata->displayName + " Build"
            : "Vehicle Build";
        CopyToBuffer(defaultName, savedBuildName.data(), savedBuildName.size());
        savedBuildNameModel = forge.modelHash;
    }

    ImGui::TextColored(EmberRed, "VEHICLE");
    ImGui::SameLine();
    ImGui::TextDisabled("- Devils Forge");
    MedievalDivider();

    if (!ImGui::BeginTabBar("##DevilsForgeTabs"))
        return;

    if (ImGui::BeginTabItem("SPAWNER")) {
        std::array<std::size_t, GTA_Vehicle_Class_Names.size()> classCounts{};
        std::size_t unresolvedClasses = 0;
        for (const auto& entry : catalog) {
            if (entry.vehicleClass >= 0 && entry.vehicleClass < static_cast<int>(classCounts.size()))
                ++classCounts[static_cast<std::size_t>(entry.vehicleClass)];
            else
                ++unresolvedClasses;
        }

        ImGui::TextColored(Bronze, "ENHANCED VEHICLE CATALOG");
        ImGui::SetNextItemWidth(310.0F);
        ImGui::InputTextWithHint("##VehicleSearch", "Search make, vehicle, or model...",
            m_vehicleSearch.data(), m_vehicleSearch.size());
        ImGui::SameLine();

        std::string classPreview = "All Vehicles (" + std::to_string(catalog.size()) + ")";
        if (m_vehicleClassFilter >= 0 && m_vehicleClassFilter < static_cast<int>(GTA_Vehicle_Class_Names.size())) {
            const auto index = static_cast<std::size_t>(m_vehicleClassFilter);
            classPreview = std::string(GTA_Vehicle_Class_Names[index]) + " (" + std::to_string(classCounts[index]) + ")";
        }

        ImGui::SetNextItemWidth(220.0F);
        if (ImGui::BeginCombo("##VehicleClass", classPreview.c_str())) {
            const std::string allLabel = "All Vehicles (" + std::to_string(catalog.size()) + ")";
            if (ImGui::Selectable(allLabel.c_str(), m_vehicleClassFilter < 0)) m_vehicleClassFilter = -1;
            for (std::size_t i = 0; i < GTA_Vehicle_Class_Names.size(); ++i) {
                const std::string label = std::string(GTA_Vehicle_Class_Names[i]) + " (" + std::to_string(classCounts[i]) + ")";
                if (ImGui::Selectable(label.c_str(), m_vehicleClassFilter == static_cast<int>(i)))
                    m_vehicleClassFilter = static_cast<int>(i);
            }
            ImGui::EndCombo();
        }

        if (unresolvedClasses > 0)
            ImGui::TextDisabled("Class metadata loading: %zu remaining. All Vehicles stays usable.", unresolvedClasses);

        const std::string_view search(m_vehicleSearch.data());
        const GTA_Vehicle_Metadata* selected = nullptr;
        const GTA_Vehicle_Metadata* firstVisible = nullptr;
        if (ImGui::BeginChild("##VehicleCatalog", ImVec2(410.0F, 270.0F), ImGuiChildFlags_Borders)) {
            for (const auto& entry : catalog) {
                if (m_vehicleClassFilter >= 0 && entry.vehicleClass != m_vehicleClassFilter)
                    continue;
                if (!ContainsInsensitive(entry.displayName, search) && !ContainsInsensitive(entry.makeName, search) &&
                    !ContainsInsensitive(entry.modelName, search))
                    continue;
                if (!firstVisible) firstVisible = &entry;
                const std::string label = entry.makeName.empty() ? entry.displayName : entry.makeName + "  " + entry.displayName;
                if (ImGui::Selectable(label.c_str(), m_selectedVehicleModel == entry.modelHash))
                    m_selectedVehicleModel = entry.modelHash;
            }
        }
        ImGui::EndChild();

        for (const auto& entry : catalog) {
