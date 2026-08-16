namespace Devilz::Frontend
{
void Devils_Den_Menu::DrawWorldPage()
{
    using namespace Devilz::Integrations::GTA5_Enhanced;

    ImGui::TextColored(EmberRed, "WORLD");
    ImGui::SameLine();
    ImGui::TextDisabled("- environment controls");
    MedievalDivider();

    ImGui::TextColored(Bronze, "PED CONTROL");
    auto& pedControl = GTA_Ped_Control_State::Instance();
    float pedRadius = pedControl.Radius();
    ImGui::SetNextItemWidth(300.0F);
    if (ImGui::SliderFloat("Radius##PedControl", &pedRadius, 10.0F, 1000.0F, "%.0f m"))
        pedControl.SetRadius(pedRadius);

    if (ImGui::Button("KILL ENEMIES", ImVec2(180.0F, 38.0F)))
        pedControl.RequestKillEnemies();
    ImGui::SameLine();
    if (ImGui::Button("KILL PEDS", ImVec2(180.0F, 38.0F)))
        pedControl.RequestKillPeds();

    const auto pedSnapshot = pedControl.Snapshot();
    if (pedSnapshot.action != GTA_Ped_Action::None) {
        if (pedSnapshot.poolReady)
            ImGui::TextDisabled("Last action: %u NPC(s) affected, %u player(s) protected/skipped.", pedSnapshot.affected, pedSnapshot.skippedPlayers);
        else
            ImGui::TextColored(EmberRed, "Ped pool unavailable - action failed closed; no entity was modified.");
    } else {
        ImGui::TextDisabled("NPC-only actions. Your ped and all network player peds are excluded.");
    }

    MedievalDivider();
    ImGui::TextColored(Bronze, "NETWORK TIME");
    int hour = NetworkTimeHour();
    int minute = NetworkTimeMinute();
    int second = NetworkTimeSecond();
    ImGui::SetNextItemWidth(260.0F);
    if (ImGui::SliderInt("Hour", &hour, 0, 23)) SetNetworkTimeSelection(hour, minute, second);
    ImGui::SetNextItemWidth(260.0F);
    if (ImGui::SliderInt("Minute", &minute, 0, 59)) SetNetworkTimeSelection(hour, minute, second);
    ImGui::SetNextItemWidth(260.0F);
    if (ImGui::SliderInt("Second", &second, 0, 59)) SetNetworkTimeSelection(hour, minute, second);
    if (ImGui::Button("SET NETWORK TIME", ImVec2(190.0F, 38.0F))) RequestSetNetworkTime();
    ImGui::SameLine();
    bool freezeTime = FreezeNetworkTime();
    if (ImGui::Checkbox("Freeze Network Time", &freezeTime)) SetFreezeNetworkTime(freezeTime);
    ImGui::TextDisabled("Selected: %02d:%02d:%02d", NetworkTimeHour(), NetworkTimeMinute(), NetworkTimeSecond());
    ImGui::TextDisabled("Turning Freeze off clears the network clock override.");

    MedievalDivider();
    ImGui::TextColored(Bronze, "WEATHER");
    int weather = WorldWeatherSelection();
    ImGui::SetNextItemWidth(300.0F);
    if (ImGui::BeginCombo("Weather", WorldWeatherLabel(weather))) {
        for (int index = 0; index < GTA_World_Weather_Count; ++index) {
            const bool selected = weather == index;
            if (ImGui::Selectable(WorldWeatherLabel(index), selected)) { weather = index; SetWorldWeatherSelection(index); }
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    if (ImGui::Button("SET WEATHER", ImVec2(150.0F, 38.0F))) RequestSetWorldWeather();
    ImGui::SameLine();
    bool forceWeather = ForceWorldWeather();
    if (ImGui::Checkbox("Force Weather", &forceWeather)) SetForceWorldWeather(forceWeather);
    ImGui::SameLine();
    if (ImGui::Button("RESET WEATHER", ImVec2(150.0F, 38.0F))) RequestResetWorldWeather();
    ImGui::TextDisabled("Weather overrides are local to your game. Force Weather reapplies the selected type every tick.");

    MedievalDivider();
    ImGui::TextColored(Bronze, "REQUEST SERVICES");
    ImGui::TextWrapped("Queue GTA Online service requests through the Enhanced 1.73 game-thread runtime. Ownership, setup, location, and freemode availability requirements still apply.");
    ImGui::TextDisabled("Ballistic delivery, instant equip, and instant remove are separate actions. SuperVolito selection is applied before its helicopter request.");
    ImGui::Spacing();

    const auto serviceAction = WorldServiceRequestSnapshot();
    ImGui::TextUnformatted("STATUS"); ImGui::SameLine();
    if (serviceAction.status == GTA_Service_Request_Status::Succeeded)
        ImGui::TextColored(ImVec4{0.42F, 0.78F, 0.42F, 1.0F}, "%s", GTA_Service_Request_Status_Name(serviceAction.status));
    else if (serviceAction.status == GTA_Service_Request_Status::Idle || serviceAction.status == GTA_Service_Request_Status::Queued)
        ImGui::TextColored(Bronze, "%s", GTA_Service_Request_Status_Name(serviceAction.status));
    else
        ImGui::TextColored(EmberRed, "%s", GTA_Service_Request_Status_Name(serviceAction.status));
    if (!serviceAction.detail.empty()) ImGui::TextWrapped("%s", serviceAction.detail.c_str());
    else ImGui::TextDisabled("Choose a service to queue its request.");

    struct ServiceButton { const char* label; GTA_Service_Request request; };
    constexpr std::array ownedServices{
        ServiceButton{"MOC", GTA_Service_Request::MobileOperationsCenter}, ServiceButton{"AVENGER", GTA_Service_Request::Avenger},
        ServiceButton{"TERRORBYTE", GTA_Service_Request::Terrorbyte}, ServiceButton{"KOSATKA", GTA_Service_Request::Kosatka},
        ServiceButton{"DINGHY", GTA_Service_Request::Dinghy}, ServiceButton{"ACID LAB", GTA_Service_Request::AcidLab},
        ServiceButton{"ACID LAB BIKE", GTA_Service_Request::AcidLabBike}, ServiceButton{"BAIL OFFICE TRANSPORTER", GTA_Service_Request::BailOfficeTransporter}
    };
    constexpr std::array supportServices{
        ServiceButton{"AMMO DROP", GTA_Service_Request::AmmoDrop}, ServiceButton{"TAXI", GTA_Service_Request::Taxi},
        ServiceButton{"BOAT PICKUP", GTA_Service_Request::BoatPickup}, ServiceButton{"HELICOPTER PICKUP", GTA_Service_Request::HelicopterPickup},
        ServiceButton{"SUPER VOLITO PICKUP", GTA_Service_Request::SuperVolitoPickup}, ServiceButton{"BULL SHARK TESTOSTERONE", GTA_Service_Request::BullSharkTestosterone},
        ServiceButton{"BACKUP HELICOPTER", GTA_Service_Request::BackupHelicopter}, ServiceButton{"CAYO HELICOPTER BACKUP", GTA_Service_Request::CayoHelicopterBackup},
        ServiceButton{"AIRSTRIKE", GTA_Service_Request::Airstrike}
    };
    constexpr std::array remoteServices{
        ServiceButton{"RC BANDITO", GTA_Service_Request::RcBandito}, ServiceButton{"RC TANK", GTA_Service_Request::RcTank},
        ServiceButton{"BALLISTIC DELIVERY", GTA_Service_Request::BallisticEquipment}, ServiceButton{"BALLISTIC INSTANT EQUIP", GTA_Service_Request::BallisticEquipmentEquip},
        ServiceButton{"BALLISTIC INSTANT REMOVE", GTA_Service_Request::BallisticEquipmentRemove}
    };
    const auto drawServiceGrid = [](const char* id, const auto& services) {
        constexpr ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchSame;
        if (!ImGui::BeginTable(id, 3, flags)) return;
        for (const auto& service : services) {
            ImGui::TableNextColumn(); ImGui::PushID(static_cast<int>(service.request));
            if (ImGui::Button(service.label, ImVec2{-1.0F, 34.0F})) (void)RequestWorldService(service.request);
            ImGui::PopID();
        }
        ImGui::EndTable();
    };
    const bool serviceQueued = serviceAction.status == GTA_Service_Request_Status::Queued;
    if (serviceQueued) ImGui::TextDisabled("Waiting for the GTA game thread...");
    ImGui::BeginDisabled(serviceQueued);
    ImGui::Spacing(); ImGui::TextDisabled("OWNED SERVICE VEHICLES"); drawServiceGrid("##OwnedServiceRequests", ownedServices);
    ImGui::Spacing(); ImGui::TextDisabled("FIELD SUPPORT"); drawServiceGrid("##SupportServiceRequests", supportServices);
    ImGui::Spacing(); ImGui::TextDisabled("REMOTE AND BALLISTIC"); drawServiceGrid("##RemoteServiceRequests", remoteServices);
    ImGui::EndDisabled();
}

void Devils_Den_Menu::DrawPlaceholderPage(const char* title, const char* detail)
{
    if (title && std::string_view(title) == "WORLD") { DrawWorldPage(); return; }
    DrawPlaceholderPageLegacy(title, detail);
}
}
