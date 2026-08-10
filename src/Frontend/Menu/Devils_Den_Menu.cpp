#include "Devils_Den_Menu.hpp"

#include "Integrations/GTA5_Enhanced/Runtime/GTA_Gameplay_State.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Teleport_Locations.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Vehicle_Catalog.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Vehicle_State.hpp"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <string>
#include <string_view>
#include <vector>

namespace Devilz::Frontend
{
namespace
{
using namespace Integrations::GTA5_Enhanced;

constexpr ImVec4 EmberRed{0.88F, 0.10F, 0.045F, 1.00F};
constexpr ImVec4 Bronze{0.78F, 0.66F, 0.44F, 1.00F};
constexpr ImVec4 Iron{0.13F, 0.11F, 0.10F, 1.00F};
constexpr ImVec4 DeepStone{0.055F, 0.045F, 0.040F, 1.00F};

struct NamedValue
{
    int value;
    const char* name;
};

constexpr std::array<NamedValue, 13> WheelTypes{{
    {0, "Sport"}, {1, "Muscle"}, {2, "Lowrider"}, {3, "SUV"}, {4, "Offroad"},
    {5, "Tuner"}, {6, "Bike Wheels"}, {7, "High End"}, {8, "Benny's Originals"},
    {9, "Benny's Bespoke"}, {10, "Open Wheel"}, {11, "Street"}, {12, "Track"}
}};

constexpr std::array<NamedValue, 13> PlateStyles{{
    {0, "Blue on White 1"}, {1, "Yellow on Black"}, {2, "Yellow on Blue"},
    {3, "Blue on White 2"}, {4, "Blue on White 3"}, {5, "Yankton"},
    {6, "Ecola"}, {7, "Las Venturas"}, {8, "Liberty City"},
    {9, "Los Santos Car Meet"}, {10, "Los Santos Panic"},
    {11, "Los Santos Pounders"}, {12, "Sprunk"}
}};

constexpr std::array<NamedValue, 7> WindowTints{{
    {0, "None"}, {1, "Pure Black"}, {2, "Dark Smoke"}, {3, "Light Smoke"},
    {4, "Stock"}, {5, "Limo"}, {6, "Green"}
}};

const char* LookupNamedValue(const auto& values, int value, const char* fallback) noexcept
{
    for (const auto& entry : values)
        if (entry.value == value)
            return entry.name;
    return fallback;
}

const char* TeleportStatusText(GTA_Teleport_Waypoint_Status status) noexcept
{
    switch (status) {
    case GTA_Teleport_Waypoint_Status::Idle: return "Ready";
    case GTA_Teleport_Waypoint_Status::Queued: return "Queued";
    case GTA_Teleport_Waypoint_Status::Resolving: return "Resolving terrain...";
    case GTA_Teleport_Waypoint_Status::Succeeded: return "Teleport complete";
    case GTA_Teleport_Waypoint_Status::NoWaypoint: return "No waypoint is active";
    case GTA_Teleport_Waypoint_Status::Failed: return "Teleport failed - see runtime log";
    default: return "Unknown";
    }
}

const char* VehicleSpawnStatusText(GTA_Vehicle_Spawn_Status status) noexcept
{
    switch (status) {
    case GTA_Vehicle_Spawn_Status::Idle: return "READY";
    case GTA_Vehicle_Spawn_Status::Queued: return "QUEUED";
    case GTA_Vehicle_Spawn_Status::Validating: return "VALIDATING";
    case GTA_Vehicle_Spawn_Status::Streaming: return "LOADING MODEL";
    case GTA_Vehicle_Spawn_Status::Creating: return "CREATING";
    case GTA_Vehicle_Spawn_Status::Applying: return "APPLYING OPTIONS";
    case GTA_Vehicle_Spawn_Status::Succeeded: return "SPAWNED";
    case GTA_Vehicle_Spawn_Status::Failed: return "FAILED - SEE LOG";
    default: return "UNKNOWN";
    }
}

bool ContainsInsensitive(std::string_view haystack, std::string_view needle)
{
    if (needle.empty())
        return true;
    std::string lhs(haystack);
    std::string rhs(needle);
    const auto lower = [](unsigned char c) { return static_cast<char>(std::tolower(c)); };
    std::transform(lhs.begin(), lhs.end(), lhs.begin(), lower);
    std::transform(rhs.begin(), rhs.end(), rhs.begin(), lower);
    return lhs.find(rhs) != std::string::npos;
}

void MedievalDivider()
{
    const auto start = ImGui::GetCursorScreenPos();
    const auto width = ImGui::GetContentRegionAvail().x;
    auto* draw = ImGui::GetWindowDrawList();
    draw->AddLine(ImVec2(start.x, start.y + 4.0F), ImVec2(start.x + width, start.y + 4.0F),
        ImGui::GetColorU32(ImVec4(0.42F, 0.08F, 0.05F, 0.95F)), 2.0F);
    draw->AddCircleFilled(ImVec2(start.x + width * 0.5F, start.y + 4.0F), 4.0F, ImGui::GetColorU32(EmberRed));
    ImGui::Dummy(ImVec2(0.0F, 11.0F));
}
}

void Devils_Den_Menu::Draw(bool& open)
{
    if (!open)
        return;

    ImGui::SetNextWindowSize(ImVec2(920.0F, 620.0F), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.985F);
    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    if (!ImGui::Begin("##DevilsDenRoot", &open, flags)) {
        ImGui::End();
        return;
    }

    DrawBanner();
    MedievalDivider();

    if (ImGui::BeginChild("##DevilsDenNavigation", ImVec2(180.0F, 0.0F), ImGuiChildFlags_Borders))
        DrawNavigation();
    ImGui::EndChild();
    ImGui::SameLine();

    if (ImGui::BeginChild("##DevilsDenContent", ImVec2(0.0F, 0.0F), ImGuiChildFlags_Borders)) {
        switch (m_page) {
        case Page::Self: DrawSelfPage(); break;
        case Page::Weapons: DrawPlaceholderPage("WEAPONS", "The armory page will inherit this same medieval frame."); break;
        case Page::Vehicle: DrawVehiclePage(); break;
        case Page::Teleport: DrawTeleportPage(); break;
        case Page::World: DrawPlaceholderPage("WORLD", "World and environment controls will live here."); break;
        case Page::Settings: DrawPlaceholderPage("SETTINGS", "Theme, hotkeys, configuration, and diagnostics will live here."); break;
        }
    }
    ImGui::EndChild();
    ImGui::End();
}

void Devils_Den_Menu::DrawBanner()
{
    const auto origin = ImGui::GetCursorScreenPos();
    const auto width = ImGui::GetContentRegionAvail().x;
    constexpr float height = 94.0F;
    auto* draw = ImGui::GetWindowDrawList();

    draw->AddRectFilled(origin, ImVec2(origin.x + width, origin.y + height),
        ImGui::GetColorU32(ImVec4(0.055F, 0.020F, 0.018F, 1.00F)), 2.0F);
    draw->AddRect(origin, ImVec2(origin.x + width, origin.y + height),
        ImGui::GetColorU32(ImVec4(0.48F, 0.09F, 0.055F, 1.00F)), 2.0F, 0, 2.0F);

    for (int i = 0; i < 5; ++i) {
        const float inset = 7.0F + static_cast<float>(i) * 6.0F;
        draw->AddLine(ImVec2(origin.x + inset, origin.y + 14.0F),
            ImVec2(origin.x + inset + 24.0F, origin.y + height - 14.0F),
            ImGui::GetColorU32(ImVec4(0.22F, 0.055F, 0.038F, 0.45F)), 1.0F);
        draw->AddLine(ImVec2(origin.x + width - inset, origin.y + 14.0F),
            ImVec2(origin.x + width - inset - 24.0F, origin.y + height - 14.0F),
            ImGui::GetColorU32(ImVec4(0.22F, 0.055F, 0.038F, 0.45F)), 1.0F);
    }

    const char* title = "DEVILS DEN MENU";
    const auto titleSize = ImGui::CalcTextSize(title);
    const auto titlePos = ImVec2(origin.x + (width - titleSize.x) * 0.5F, origin.y + 27.0F);
    draw->AddText(ImVec2(titlePos.x + 2.0F, titlePos.y + 2.0F), ImGui::GetColorU32(ImVec4(0, 0, 0, 0.85F)), title);
    draw->AddText(titlePos, ImGui::GetColorU32(EmberRed), title);

    const char* subtitle = "GTA V ENHANCED  |  RUNTIME READY";
    const auto subtitleSize = ImGui::CalcTextSize(subtitle);
    draw->AddText(ImVec2(origin.x + (width - subtitleSize.x) * 0.5F, origin.y + 56.0F),
        ImGui::GetColorU32(Bronze), subtitle);
    ImGui::Dummy(ImVec2(width, height));
}

void Devils_Den_Menu::DrawNavigation()
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, DeepStone);
    ImGui::TextColored(Bronze, "CATEGORIES");
    MedievalDivider();

    struct NavigationEntry { const char* label; Page page; };
    constexpr std::array entries{
        NavigationEntry{"SELF", Page::Self}, NavigationEntry{"WEAPONS", Page::Weapons},
        NavigationEntry{"VEHICLE", Page::Vehicle}, NavigationEntry{"TELEPORT", Page::Teleport},
        NavigationEntry{"WORLD", Page::World}, NavigationEntry{"SETTINGS", Page::Settings}
    };

    for (const auto& entry : entries) {
        const bool selected = m_page == entry.page;
        ImGui::PushStyleColor(ImGuiCol_Button, selected ? ImVec4(0.38F, 0.055F, 0.035F, 1.00F) : Iron);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
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

    m_godMode = gameplay.GodMode(); if (ImGui::Checkbox("God Mode", &m_godMode)) gameplay.SetGodMode(m_godMode);
    m_neverWanted = gameplay.NeverWanted(); if (ImGui::Checkbox("Never Wanted", &m_neverWanted)) gameplay.SetNeverWanted(m_neverWanted);
    m_superJump = gameplay.SuperJump(); if (ImGui::Checkbox("Super Jump", &m_superJump)) gameplay.SetSuperJump(m_superJump);
    m_infiniteOxygen = gameplay.InfiniteOxygen(); if (ImGui::Checkbox("Infinite Oxygen", &m_infiniteOxygen)) gameplay.SetInfiniteOxygen(m_infiniteOxygen);
    m_noRagdoll = gameplay.NoRagdoll(); if (ImGui::Checkbox("No Ragdoll", &m_noRagdoll)) gameplay.SetNoRagdoll(m_noRagdoll);
    m_keepPlayerClean = gameplay.KeepPlayerClean(); if (ImGui::Checkbox("Keep Player Clean", &m_keepPlayerClean)) gameplay.SetKeepPlayerClean(m_keepPlayerClean);
    m_infiniteAmmo = gameplay.InfiniteAmmo(); if (ImGui::Checkbox("Infinite Ammo", &m_infiniteAmmo)) gameplay.SetInfiniteAmmo(m_infiniteAmmo);

    ImGui::BeginDisabled();
    ImGui::Checkbox("Fast Run", &m_fastRun);
    ImGui::SliderFloat("Health", &m_health, 0.0F, 100.0F, "%.0f");
    ImGui::EndDisabled();

    MedievalDivider();
    ImGui::TextColored(Bronze, "WEAPONS / ACTIONS");
    if (ImGui::Button("GIVE ALL WEAPONS", ImVec2(190.0F, 38.0F))) gameplay.RequestGiveAllWeapons();
    ImGui::SameLine();
    if (ImGui::Button("GIVE MAX AMMO", ImVec2(190.0F, 38.0F))) gameplay.RequestGiveMaxAmmo();
    ImGui::TextDisabled("Live Self features execute only from the validated RunScriptThreads game-thread context.");
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
        if (frontWheels != forge.categories.end())
            m_forgeWheelIndex = frontWheels->installedIndex;
    }

    ImGui::TextColored(EmberRed, "VEHICLE");
    ImGui::SameLine();
    ImGui::TextDisabled("- Devils Forge");
    MedievalDivider();

    if (!ImGui::BeginTabBar("##DevilsForgeTabs"))
        return;

    if (ImGui::BeginTabItem("SPAWNER")) {
        ImGui::TextColored(Bronze, "ENHANCED VEHICLE CATALOG");
        ImGui::SetNextItemWidth(310.0F);
        ImGui::InputTextWithHint("##VehicleSearch", "Search make, vehicle, or model...", m_vehicleSearch.data(), m_vehicleSearch.size());
        ImGui::SameLine();

        const char* classPreview = m_vehicleClassFilter < 0 ? "All Classes" :
            GTA_Vehicle_Class_Names[static_cast<std::size_t>(m_vehicleClassFilter)].data();
        ImGui::SetNextItemWidth(190.0F);
        if (ImGui::BeginCombo("##VehicleClass", classPreview)) {
            if (ImGui::Selectable("All Classes", m_vehicleClassFilter < 0)) m_vehicleClassFilter = -1;
            for (std::size_t i = 0; i < GTA_Vehicle_Class_Names.size(); ++i) {
                if (ImGui::Selectable(GTA_Vehicle_Class_Names[i].data(), m_vehicleClassFilter == static_cast<int>(i)))
                    m_vehicleClassFilter = static_cast<int>(i);
            }
            ImGui::EndCombo();
        }

        const std::string_view search(m_vehicleSearch.data());
        const GTA_Vehicle_Metadata* selected = nullptr;
        if (ImGui::BeginChild("##VehicleCatalog", ImVec2(390.0F, 245.0F), ImGuiChildFlags_Borders)) {
            for (const auto& entry : catalog) {
                if (m_vehicleClassFilter >= 0 && entry.vehicleClass != m_vehicleClassFilter)
                    continue;
                if (!ContainsInsensitive(entry.displayName, search) && !ContainsInsensitive(entry.makeName, search) &&
                    !ContainsInsensitive(entry.modelName, search))
                    continue;

                const std::string label = entry.makeName.empty() ? entry.displayName : entry.makeName + "  " + entry.displayName;
                if (ImGui::Selectable(label.c_str(), m_selectedVehicleModel == entry.modelHash))
                    m_selectedVehicleModel = entry.modelHash;
            }
        }
        ImGui::EndChild();

        for (const auto& entry : catalog) {
            if (entry.modelHash == m_selectedVehicleModel) { selected = &entry; break; }
        }
        if (!selected && !catalog.empty()) {
            m_selectedVehicleModel = catalog.front().modelHash;
            selected = &catalog.front();
        }

        ImGui::SameLine();
        if (ImGui::BeginChild("##VehicleDetails", ImVec2(0.0F, 245.0F), ImGuiChildFlags_Borders)) {
            ImGui::TextColored(Bronze, "SELECTED VEHICLE");
            if (selected) {
                ImGui::TextWrapped("%s", selected->displayName.c_str());
                if (!selected->makeName.empty()) ImGui::TextDisabled("%s", selected->makeName.c_str());
                ImGui::Text("Model: %s", selected->modelName.c_str());
                if (selected->vehicleClass >= 0 && selected->vehicleClass < static_cast<int>(GTA_Vehicle_Class_Names.size()))
                    ImGui::Text("Class: %s", GTA_Vehicle_Class_Names[static_cast<std::size_t>(selected->vehicleClass)].data());
                ImGui::Text("Hash: 0x%08X", selected->modelHash);
            } else {
                ImGui::TextDisabled("Catalog metadata is loading on the game thread...");
            }
            ImGui::Separator();
            ImGui::Checkbox("Spawn Inside", &m_spawnInsideVehicle);
            ImGui::Checkbox("Spawn Maxed", &m_spawnVehicleMaxed);
            ImGui::Checkbox("Place On Ground", &m_spawnVehicleOnGround);
            ImGui::Checkbox("Engine Running", &m_spawnVehicleEngineRunning);
            ImGui::Checkbox("Invincible", &m_spawnVehicleInvincible);
            ImGui::Checkbox("Clean", &m_spawnVehicleClean);
        }
        ImGui::EndChild();

        const auto status = vehicles.SpawnStatus();
        const bool busy = status == GTA_Vehicle_Spawn_Status::Queued || status == GTA_Vehicle_Spawn_Status::Validating ||
            status == GTA_Vehicle_Spawn_Status::Streaming || status == GTA_Vehicle_Spawn_Status::Creating ||
            status == GTA_Vehicle_Spawn_Status::Applying;
        ImGui::TextColored(Bronze, "STATUS");
        ImGui::SameLine();
        ImGui::TextUnformatted(VehicleSpawnStatusText(status));
        ImGui::BeginDisabled(busy || !selected);
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

    if (ImGui::BeginTabItem("DEVILS FORGE")) {
        ImGui::TextColored(Bronze, "CURRENT VEHICLE");
        const GTA_Vehicle_Metadata* currentMetadata = nullptr;
        for (const auto& entry : catalog) {
            if (entry.modelHash == forge.modelHash) {
                currentMetadata = &entry;
                break;
            }
        }

        if (forge.vehicle == 0) {
            ImGui::TextDisabled("Enter a vehicle to unlock Devils Forge.");
            if (ImGui::Button("REFRESH VEHICLE DATA", ImVec2(210.0F, 34.0F)))
                vehicles.RequestForgeSnapshotRefresh();
            ImGui::EndTabItem();
            ImGui::EndTabBar();
            return;
        }

        if (currentMetadata) {
            if (!currentMetadata->makeName.empty()) {
                ImGui::Text("%s %s", currentMetadata->makeName.c_str(), currentMetadata->displayName.c_str());
            } else {
                ImGui::Text("%s", currentMetadata->displayName.c_str());
            }
            ImGui::TextDisabled("Model: %s  |  Hash: 0x%08X", currentMetadata->modelName.c_str(), forge.modelHash);
        } else {
            ImGui::Text("Vehicle 0x%08X", forge.modelHash);
        }
        ImGui::SameLine();
        if (ImGui::Button("REFRESH", ImVec2(90.0F, 28.0F)))
            vehicles.RequestForgeSnapshotRefresh();

        MedievalDivider();
        ImGui::TextColored(Bronze, "CONDITION / PROTECTION");
        bool keepPerfect = vehicles.KeepVehiclePerfect();
        if (ImGui::Checkbox("Always Keep Vehicle Perfect", &keepPerfect))
            vehicles.SetKeepVehiclePerfect(keepPerfect);
        ImGui::SameLine();
        ImGui::TextDisabled("Repairs damage, deformation, engine/body health and dirt continuously");

        bool vehicleGodMode = vehicles.VehicleGodMode();
        if (ImGui::Checkbox("Vehicle God Mode", &vehicleGodMode))
            vehicles.SetVehicleGodMode(vehicleGodMode);
        ImGui::SameLine();
        ImGui::TextDisabled("Current occupied vehicle only");

        if (ImGui::Button("REPAIR EVERYTHING", ImVec2(170.0F, 34.0F)))
            vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::RepairVehicle, 0, 0, 0});
        ImGui::SameLine();
        if (ImGui::Button("CLEAN VEHICLE", ImVec2(150.0F, 34.0F)))
            vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::CleanVehicle, 0, 0, 0});
        ImGui::SameLine();
        if (ImGui::Checkbox("Quick Lowered Stance", &m_forgeLoweredStance))
            vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::SetLoweredStance, m_forgeLoweredStance ? 1 : 0, 0, 0});

        MedievalDivider();
        ImGui::TextColored(Bronze, "VEHICLE-SPECIFIC MODS");
        ImGui::TextDisabled("Only categories supported by the current vehicle are shown. Selecting an option applies it immediately.");

        const GTA_Vehicle_Forge_Category* selectedCategory = nullptr;
        for (const auto& category : forge.categories) {
            if (category.slot == m_forgeModSlot) {
                selectedCategory = &category;
                break;
            }
        }

        if (ImGui::BeginChild("##ForgeCategories", ImVec2(225.0F, 220.0F), ImGuiChildFlags_Borders)) {
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
        if (ImGui::BeginChild("##ForgeOptions", ImVec2(0.0F, 220.0F), ImGuiChildFlags_Borders)) {
            ImGui::TextColored(Bronze, "OPTIONS");
            if (!selectedCategory) {
                ImGui::TextDisabled("Choose a supported category.");
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

        if (ImGui::Button("TURBO ON", ImVec2(110.0F, 32.0F)))
            vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::ToggleMod, 18, 1, 0});
        ImGui::SameLine();
        if (ImGui::Button("TURBO OFF", ImVec2(110.0F, 32.0F)))
            vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::ToggleMod, 18, 0, 0});
        ImGui::SameLine();
        if (ImGui::Button("XENON ON", ImVec2(110.0F, 32.0F)))
            vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::ToggleMod, 22, 1, 0});
        ImGui::SameLine();
        if (ImGui::Button("XENON OFF", ImVec2(110.0F, 32.0F)))
            vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::ToggleMod, 22, 0, 0});

        MedievalDivider();
        ImGui::TextColored(Bronze, "WHEELS");
        ImGui::SetNextItemWidth(240.0F);
        const char* wheelPreview = LookupNamedValue(WheelTypes, m_forgeWheelType, "Unknown");
        if (ImGui::BeginCombo("Wheel Category", wheelPreview)) {
            for (const auto& wheelType : WheelTypes) {
                const bool selected = m_forgeWheelType == wheelType.value;
                if (ImGui::Selectable(wheelType.name, selected)) {
                    m_forgeWheelType = wheelType.value;
                    m_forgeWheelIndex = -1;
                    vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::SetWheelType, wheelType.value, 0, 0});
                }
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        ImGui::Checkbox("Custom Tires", &m_forgeCustomTires);

        const auto frontWheels = std::find_if(forge.categories.begin(), forge.categories.end(),
            [](const GTA_Vehicle_Forge_Category& category) { return category.slot == 23; });
        if (forge.wheelType != m_forgeWheelType) {
            ImGui::TextDisabled("Refreshing rim names for %s...", wheelPreview);
        } else if (frontWheels == forge.categories.end()) {
            ImGui::TextDisabled("This vehicle does not expose front-wheel modifications for this category.");
        } else if (ImGui::BeginChild("##ForgeRims", ImVec2(0.0F, 165.0F), ImGuiChildFlags_Borders)) {
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
        }

        MedievalDivider();
        ImGui::TextColored(Bronze, "WINDOWS / PLATES");
        ImGui::SetNextItemWidth(240.0F);
        const char* tintPreview = LookupNamedValue(WindowTints, m_forgeWindowTint, "Unknown");
        if (ImGui::BeginCombo("Window Tint", tintPreview)) {
            for (const auto& tint : WindowTints) {
                if (ImGui::Selectable(tint.name, m_forgeWindowTint == tint.value)) {
                    m_forgeWindowTint = tint.value;
                    vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::SetWindowTint, tint.value, 0, 0});
                }
            }
            ImGui::EndCombo();
        }

        ImGui::SetNextItemWidth(240.0F);
        const char* platePreview = LookupNamedValue(PlateStyles, m_forgePlateStyle, "Unknown");
        if (ImGui::BeginCombo("Plate Style", platePreview)) {
            for (const auto& plate : PlateStyles) {
                if (ImGui::Selectable(plate.name, m_forgePlateStyle == plate.value)) {
                    m_forgePlateStyle = plate.value;
                    vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::SetPlateStyle, plate.value, 0, 0});
                }
            }
            ImGui::EndCombo();
        }

        MedievalDivider();
        if (ImGui::CollapsingHeader("ADVANCED PAINT / RAW COLOR INDEXES")) {
            ImGui::TextDisabled("Named LSC and Chameleon palettes are the next paint pass; raw values remain available here for now.");
            constexpr std::array paintTypes{"Normal", "Metallic", "Pearl", "Matte", "Metal", "Chrome", "Chameleon"};
            ImGui::SetNextItemWidth(180.0F);
            if (ImGui::BeginCombo("Primary Type", paintTypes[static_cast<std::size_t>(std::clamp(m_forgePrimaryPaintType, 0, 6))])) {
                for (int i = 0; i < static_cast<int>(paintTypes.size()); ++i)
                    if (ImGui::Selectable(paintTypes[static_cast<std::size_t>(i)], m_forgePrimaryPaintType == i)) m_forgePrimaryPaintType = i;
                ImGui::EndCombo();
            }
            ImGui::InputInt("Primary Color Index", &m_forgePrimaryColor);
            if (ImGui::Button("APPLY PRIMARY", ImVec2(150.0F, 34.0F)))
                vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::SetPrimaryPaint,
                    m_forgePrimaryPaintType, m_forgePrimaryColor, m_forgePearlescent});

            ImGui::SetNextItemWidth(180.0F);
            if (ImGui::BeginCombo("Secondary Type", paintTypes[static_cast<std::size_t>(std::clamp(m_forgeSecondaryPaintType, 0, 6))])) {
                for (int i = 0; i < static_cast<int>(paintTypes.size()); ++i)
                    if (ImGui::Selectable(paintTypes[static_cast<std::size_t>(i)], m_forgeSecondaryPaintType == i)) m_forgeSecondaryPaintType = i;
                ImGui::EndCombo();
            }
            ImGui::InputInt("Secondary Color Index", &m_forgeSecondaryColor);
            if (ImGui::Button("APPLY SECONDARY", ImVec2(150.0F, 34.0F)))
                vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::SetSecondaryPaint,
                    m_forgeSecondaryPaintType, m_forgeSecondaryColor, 0});

            ImGui::InputInt("Pearlescent Index", &m_forgePearlescent);
            ImGui::InputInt("Wheel Color Index", &m_forgeWheelColor);
            if (ImGui::Button("APPLY PEARL + WHEEL COLOR", ImVec2(230.0F, 34.0F)))
                vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::SetExtraColours,
                    m_forgePearlescent, m_forgeWheelColor, 0});

            ImGui::InputInt3("Primary RGB", m_forgePrimaryRgb.data());
            if (ImGui::Button("APPLY PRIMARY RGB", ImVec2(180.0F, 34.0F)))
                vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::SetCustomPrimaryRgb,
                    m_forgePrimaryRgb[0], m_forgePrimaryRgb[1], m_forgePrimaryRgb[2]});
            ImGui::InputInt3("Secondary RGB", m_forgeSecondaryRgb.data());
            if (ImGui::Button("APPLY SECONDARY RGB", ImVec2(190.0F, 34.0F)))
                vehicles.QueueForgeCommand({GTA_Vehicle_Forge_Command_Type::SetCustomSecondaryRgb,
                    m_forgeSecondaryRgb[0], m_forgeSecondaryRgb[1], m_forgeSecondaryRgb[2]});
        }

        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("SAVED BUILDS")) {
        ImGui::TextColored(Bronze, "SAVED VEHICLE BUILDS");
        ImGui::TextWrapped("The save/clone JSON layer will capture the actual vehicle state after this named Forge layer is validated in-game.");
        ImGui::Spacing();
        ImGui::TextDisabled("Target: save, clone, spawn, rename, favorite, and restore complete vehicle builds.");
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("HANDLING")) {
        ImGui::TextColored(Bronze, "HANDLING EDITOR");
        ImGui::TextWrapped("YimMenu-style live sliders are reserved for the validated handling pass: ride height, suspension, traction, braking, drivetrain, steering and reset-to-factory values.");
        ImGui::Spacing();
        ImGui::TextDisabled("No guessed Enhanced handling offsets will be written.");
        ImGui::EndTabItem();
    }

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
    ImGui::TextWrapped("Place a waypoint on the Story Mode map, then use the button below. If you are inside a vehicle, the vehicle teleports with you.");

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
    ImGui::TextDisabled("Waypoint travel resolves ground, water, then approximate terrain. Current vehicles are moved with the player.");
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
