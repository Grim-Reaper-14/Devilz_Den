#include "Devils_Den_Menu.hpp"

#include "Devils_Den_Config.hpp"
#include "Frontend/Menu/Lua/Lua_Page.hpp"
#include "Frontend/Menu/Network/Network_Page.hpp"
#include "Frontend/Menu/Self/Self_Page.hpp"
#include "Frontend/Menu/Themes/Menu_Theme.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Gameplay_State.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Teleport_Locations.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Vehicle_Catalog.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Vehicle_Customization_Data.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Vehicle_Saved_Builds.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Vehicle_Personal_Save.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Vehicle_Tools.hpp"
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

constexpr NamedValue ExplosionTypes[]{
    {-1, "Don't Care"}, {0, "Grenade"}, {1, "Grenade Launcher"}, {2, "Sticky Bomb"},
    {3, "Molotov"}, {4, "Rocket"}, {5, "Tank Shell"}, {6, "Hi Octane"}, {7, "Car"},
    {8, "Plane"}, {9, "Petrol Pump"}, {10, "Bike"}, {11, "Directional Steam"},
    {12, "Directional Flame"}, {13, "Directional Water Hydrant"}, {14, "Directional Gas Canister"},
    {15, "Boat"}, {16, "Ship Destroy"}, {17, "Truck"}, {18, "Bullet"},
    {19, "Smoke Grenade Launcher"}, {20, "Smoke Grenade"}, {21, "BZ Gas"}, {22, "Flare"},
    {23, "Gas Canister"}, {24, "Extinguisher"}, {25, "Programmable AR"}, {26, "Train"},
    {27, "Barrel"}, {28, "Propane"}, {29, "Blimp"}, {30, "Directional Flame Explode"},
    {31, "Tanker"}, {32, "Plane Rocket"}, {33, "Vehicle Bullet"}, {34, "Gas Tank"},
    {35, "Bird Crap"}, {36, "Railgun"}, {37, "Blimp 2"}, {38, "Firework"}, {39, "Snowball"},
    {40, "Proximity Mine"}, {41, "Valkyrie Cannon"}, {42, "Air Defence"}, {43, "Pipe Bomb"},
    {44, "Vehicle Mine"}, {45, "Explosive Ammo"}, {46, "APC Shell"}, {47, "Cluster Bomb"},
    {48, "Gas Bomb"}, {49, "Incendiary Bomb"}, {50, "Standard Bomb"}, {51, "Torpedo"},
    {52, "Underwater Torpedo"}, {53, "Bombushka Cannon"}, {54, "Cluster Bomb (Secondary)"},
    {55, "Hunter Barrage"}, {56, "Hunter Cannon"}, {57, "Rogue Cannon"}, {58, "Underwater Mine"},
    {59, "Orbital Cannon"}, {60, "Standard Bomb (Wide)"}, {61, "Explosive Ammo (Shotgun)"},
    {62, "Oppressor Mk II Cannon"}, {63, "Kinetic Mortar"}, {64, "Vehicle Mine (Kinetic)"},
    {65, "Vehicle Mine (EMP)"}, {66, "Vehicle Mine (Spike)"}, {67, "Vehicle Mine (Slick)"},
    {68, "Vehicle Mine (Tar)"}, {69, "Script Drone"}, {70, "Ray Gun"}, {71, "Buried Mine"},
    {72, "Script Missile"}, {73, "RC Tank Rocket"}, {74, "Water Bomb"}, {75, "Water Bomb (Secondary)"},
    {76, "Unknown Explosion 76"}, {77, "Unknown Explosion 77"}, {78, "Flash Grenade"},
    {79, "Stun Grenade"}, {80, "Unknown Explosion 80"}, {81, "Script Missile (Large)"},
    {82, "Submarine (Big)"}, {83, "EMP Launcher"}
};

const char* LookupNamedValue(const auto& values, int value, const char* fallback) noexcept
{
    for (const auto& entry : values)
        if (entry.value == value)
            return entry.name;
    return fallback;
}

const char* LookupVehicleValue(const auto& values, int value, const char* fallback = "Unknown") noexcept
{
    for (const auto& entry : values)
        if (entry.value == value)
            return entry.name.data();
    return fallback;
}

const char* PaintColorPreview(int paintType, int color) noexcept
{
    const auto name = GTA_Vehicle_Paint_Color_Name(paintType, color, "Unknown");
    return name.data();
}

int FirstPaintColor(int paintType) noexcept
{
    const auto palette = GTA_Vehicle_Paint_Palette(paintType);
    return palette.empty() ? 0 : palette.front().value;
}

bool PaintPaletteContains(int paintType, int color) noexcept
{
    const auto palette = GTA_Vehicle_Paint_Palette(paintType);
    return std::any_of(palette.begin(), palette.end(), [color](const auto& entry) {
        return entry.value == color;
    });
}

void CopyToBuffer(std::string_view value, char* buffer, std::size_t size)
{
    if (!buffer || size == 0)
        return;
    std::fill_n(buffer, size, '\0');
    const auto count = (std::min)(value.size(), size - 1U);
    std::copy_n(value.data(), count, buffer);
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

const char* CloneNearestVehicleStatusText(GTA_Vehicle_Clone_Nearest_Status status) noexcept
{
    switch (status) {
    case GTA_Vehicle_Clone_Nearest_Status::Idle: return "Ready";
    case GTA_Vehicle_Clone_Nearest_Status::Queued: return "Queued";
    case GTA_Vehicle_Clone_Nearest_Status::Searching: return "Finding nearest vehicle...";
    case GTA_Vehicle_Clone_Nearest_Status::SpawnQueued: return "Clone queued in vehicle spawner";
    case GTA_Vehicle_Clone_Nearest_Status::Succeeded: return "Nearest vehicle cloned";
    case GTA_Vehicle_Clone_Nearest_Status::NoVehicle: return "No nearby vehicle found";
    case GTA_Vehicle_Clone_Nearest_Status::Unavailable: return "Nearest-vehicle lookup unavailable";
    case GTA_Vehicle_Clone_Nearest_Status::Failed: return "Clone failed - see log";
    default: return "Unknown";
    }
}

const char* PersonalVehicleSaveStatusText(GTA_Vehicle_Personal_Save_Status status) noexcept
{
    switch (status) {
    case GTA_Vehicle_Personal_Save_Status::Idle: return "Ready";
    case GTA_Vehicle_Personal_Save_Status::Queued: return "Queued";
    case GTA_Vehicle_Personal_Save_Status::Validating: return "Checking vehicle eligibility...";
    case GTA_Vehicle_Personal_Save_Status::OpeningGarageMenu: return "Opening GTA garage flow...";
    case GTA_Vehicle_Personal_Save_Status::WaitingForGarageSelection: return "Choose a garage/slot in GTA";
    case GTA_Vehicle_Personal_Save_Status::Completed: return "Garage transaction closed";
    case GTA_Vehicle_Personal_Save_Status::NoVehicle: return "Get in the vehicle you want to save";
    case GTA_Vehicle_Personal_Save_Status::InvalidVehicle: return "GTA will not allow this vehicle as a personal vehicle";
    case GTA_Vehicle_Personal_Save_Status::AlreadyPersonalVehicle: return "This is already your active personal vehicle";
    case GTA_Vehicle_Personal_Save_Status::Unavailable: return "Personal-vehicle save flow unavailable";
    case GTA_Vehicle_Personal_Save_Status::Failed: return "Save failed - see runtime log";
    default: return "Unknown";
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
    Themes::Menu_Theme_Manager::Instance().DrawDivider();
}
}

void Devils_Den_Menu::Draw(bool& open)
{
    if (!open)
        return;

    auto& theme = Themes::Menu_Theme_Manager::Instance();
    const auto& palette = theme.Palette();
    theme.Apply();

    ImGui::SetNextWindowSize(ImVec2(1320.0F, 790.0F), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.995F);
    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    if (!ImGui::Begin("##DevilsDenRoot", &open, flags)) {
        ImGui::End();
        return;
    }

    DrawBanner();
    ImGui::Dummy(ImVec2(0.0F, 5.0F));

    constexpr float navigationWidth = 215.0F;
    constexpr float detailsWidth = 260.0F;
    constexpr float footerHeight = 52.0F;
    constexpr float spacing = 8.0F;

    const ImVec2 bodyAvailable = ImGui::GetContentRegionAvail();
    const float bodyHeight = (std::max)(300.0F, bodyAvailable.y - footerHeight - spacing);
    const float centerWidth = (std::max)(420.0F, bodyAvailable.x - navigationWidth - detailsWidth - spacing * 2.0F);
    auto* rootDraw = ImGui::GetWindowDrawList();

    const ImVec2 navMin = ImGui::GetCursorScreenPos();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4{0.030F, 0.022F, 0.020F, 0.985F});
    if (ImGui::BeginChild("##DevilsDenNavigation", ImVec2(navigationWidth, bodyHeight), ImGuiChildFlags_Borders))
        DrawNavigation();
    ImGui::EndChild();
    ImGui::PopStyleColor();
    theme.DrawOrnateFrame(rootDraw, navMin, ImVec2{navMin.x + navigationWidth, navMin.y + bodyHeight}, false);

    ImGui::SameLine(0.0F, spacing);
    const ImVec2 contentMin = ImGui::GetCursorScreenPos();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4{0.032F, 0.023F, 0.020F, 0.985F});
    if (ImGui::BeginChild("##DevilsDenContent", ImVec2(centerWidth, bodyHeight), ImGuiChildFlags_Borders)) {
        switch (m_page) {
        case Page::Self: Self::DrawSelfPage(); break;
        case Page::Weapons: DrawWeaponsPage(); break;
        case Page::Vehicle: DrawVehiclePage(); break;
        case Page::Teleport: DrawTeleportPage(); break;
        case Page::World: DrawPlaceholderPage("WORLD", "World and environment controls will live here."); break;
        case Page::Network: Network::DrawNetworkPage(); break;
        case Page::Lua: Lua::DrawLuaPage(); break;
        case Page::Settings: DrawSettingsPage(); break;
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    theme.DrawOrnateFrame(rootDraw, contentMin, ImVec2{contentMin.x + centerWidth, contentMin.y + bodyHeight}, false);

    ImGui::SameLine(0.0F, spacing);
    const ImVec2 detailsMin = ImGui::GetCursorScreenPos();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4{0.028F, 0.020F, 0.018F, 0.990F});
    if (ImGui::BeginChild("##DevilsDenDetails", ImVec2(detailsWidth, bodyHeight), ImGuiChildFlags_Borders)) {
        const char* detailTitle = "PLAYER";
        const char* detailText = "Modify player abilities, protection and movement through the live Story Mode runtime.";
        const char* detailTag = "SELF / PLAYER";
        Themes::Menu_Icon detailIcon = Themes::Menu_Icon::Self;
        switch (m_page) {
        case Page::Self:
            detailTitle = "PLAYER";
            detailText = "Player protection, wanted state, movement and live character options.";
            detailTag = "SELF / PLAYER";
            detailIcon = Themes::Menu_Icon::Self;
            break;
        case Page::Weapons:
            detailTitle = "ARMORY";
            detailText = "Weapons, ammunition, loadout actions and live bullet effects.";
            detailTag = "WEAPONS / COMBAT";
            detailIcon = Themes::Menu_Icon::Weapons;
            break;
        case Page::Vehicle:
            detailTitle = "DEVILS FORGE";
            detailText = "Spawn, customize, preserve and rebuild GTA V Enhanced vehicles.";
            detailTag = "VEHICLE / FORGE";
            detailIcon = Themes::Menu_Icon::Vehicle;
            break;
        case Page::Teleport:
            detailTitle = "WAYFINDER";
            detailText = "Waypoint and preset travel tools for the player or occupied vehicle.";
            detailTag = "TELEPORT / MAP";
            detailIcon = Themes::Menu_Icon::Teleport;
            break;
        case Page::World:
            detailTitle = "WORLD";
            detailText = "World and environment controls will live in this domain as they are added.";
            detailTag = "WORLD / ENVIRONMENT";
            detailIcon = Themes::Menu_Icon::World;
            break;
        case Page::Network:
            detailTitle = "NETWORK";
            detailText = "Sessions, players and protections are organized in the Network domain.";
            detailTag = "NETWORK / SESSIONS";
            detailIcon = Themes::Menu_Icon::Network;
            break;
        case Page::Lua:
            detailTitle = "LUA SCRIPTING";
            detailText = "Reserved for embedded Lua scripts using the same backend APIs as the native menu.";
            detailTag = "LUA / SCRIPTS";
            detailIcon = Themes::Menu_Icon::Lua;
            break;
        case Page::Settings:
            detailTitle = "SETTINGS";
            detailText = "Configuration, theme, input and future menu preferences belong here.";
            detailTag = "SETTINGS / CONFIG";
            detailIcon = Themes::Menu_Icon::Settings;
            break;
        }

        ImGui::TextColored(palette.emberRed, "OPTION DETAILS");
        MedievalDivider();
        const ImVec2 iconOrigin = ImGui::GetCursorScreenPos();
        const float iconCenterX = iconOrigin.x + ImGui::GetContentRegionAvail().x * 0.5F;
        const ImVec2 iconCenter{iconCenterX, iconOrigin.y + 43.0F};
        auto* detailDraw = ImGui::GetWindowDrawList();
        detailDraw->AddCircleFilled(iconCenter, 35.0F, ImGui::GetColorU32(ImVec4{0.075F, 0.020F, 0.017F, 1.0F}), 30);
        detailDraw->AddCircle(iconCenter, 36.0F, ImGui::GetColorU32(palette.emberGlow), 30, 3.0F);
        detailDraw->AddCircle(iconCenter, 30.0F, ImGui::GetColorU32(palette.bronzeDark), 30, 1.0F);
        theme.DrawIcon(detailIcon, detailDraw, iconCenter, 20.0F, ImGui::GetColorU32(palette.bronze));
        ImGui::Dummy(ImVec2(0.0F, 88.0F));

        const float titleWidth = ImGui::CalcTextSize(detailTitle).x;
        ImGui::SetCursorPosX((std::max)(ImGui::GetStyle().WindowPadding.x,
            (ImGui::GetWindowWidth() - titleWidth) * 0.5F));
        ImGui::TextColored(palette.emberRed, "%s", detailTitle);
        ImGui::Spacing();
        ImGui::TextWrapped("%s", detailText);
        MedievalDivider();
        ImGui::TextColored(palette.bronze, "DOMAIN");
        ImGui::TextWrapped("%s", detailTag);
        MedievalDivider();
        ImGui::TextColored(palette.bronze, "RUNTIME");
        ImGui::TextColored(ImVec4{0.42F, 0.78F, 0.42F, 1.0F}, "LIVE APPLY");
        ImGui::TextWrapped("Controls in Devils Den apply through the validated GTA V Enhanced game-thread bridge.");

        ImGui::SetCursorPosY((std::max)(ImGui::GetCursorPosY(), ImGui::GetWindowHeight() - 74.0F));
        ImGui::TextDisabled("INSERT");
        ImGui::SameLine();
        ImGui::TextColored(palette.bronze, "toggle menu");
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    theme.DrawOrnateFrame(rootDraw, detailsMin, ImVec2{detailsMin.x + detailsWidth, detailsMin.y + bodyHeight}, false);

    ImGui::Dummy(ImVec2(0.0F, spacing));
    const ImVec2 footerMin = ImGui::GetCursorScreenPos();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4{0.030F, 0.020F, 0.018F, 0.995F});
    if (ImGui::BeginChild("##DevilsDenFooter", ImVec2(0.0F, footerHeight), ImGuiChildFlags_Borders)) {
        if (ImGui::Button("CONFIGS", ImVec2(160.0F, 32.0F))) m_page = Page::Settings;
        ImGui::SameLine();
        if (ImGui::Button("PLAYER", ImVec2(160.0F, 32.0F))) m_page = Page::Self;
        ImGui::SameLine();
        if (ImGui::Button("DEVILS FORGE", ImVec2(180.0F, 32.0F))) m_page = Page::Vehicle;
        ImGui::SameLine();
        ImGui::TextColored(palette.bronze, "LIVE CHANGES");
        ImGui::SameLine();
        ImGui::TextDisabled("apply immediately");
        const float closeWidth = 150.0F;
        ImGui::SameLine((std::max)(ImGui::GetCursorPosX() + 8.0F, ImGui::GetWindowWidth() - closeWidth - 12.0F));
        if (ImGui::Button("CLOSE MENU", ImVec2(closeWidth, 32.0F))) open = false;
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    theme.DrawOrnateFrame(rootDraw, footerMin,
        ImVec2{footerMin.x + ImGui::GetContentRegionAvail().x, footerMin.y + footerHeight}, true);

    ImGui::End();
}

void Devils_Den_Menu::DrawBanner()
{
    Themes::Menu_Theme_Manager::Instance().DrawHeader(
        "DEVIL'S DEN MENU",
        "GTA V ENHANCED  |  RUNTIME READY");
}

void Devils_Den_Menu::DrawNavigation()
{
    auto& theme = Themes::Menu_Theme_Manager::Instance();
    const auto& palette = theme.Palette();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, palette.deepStone);
    ImGui::TextColored(palette.bronze, "CATEGORIES");
    MedievalDivider();

    struct NavigationEntry
    {
        const char* label;
        Page page;
        Themes::Menu_Icon icon;
    };
    constexpr std::array entries{
        NavigationEntry{"       SELF", Page::Self, Themes::Menu_Icon::Self},
        NavigationEntry{"       WEAPONS", Page::Weapons, Themes::Menu_Icon::Weapons},
        NavigationEntry{"       VEHICLE", Page::Vehicle, Themes::Menu_Icon::Vehicle},
        NavigationEntry{"       TELEPORT", Page::Teleport, Themes::Menu_Icon::Teleport},
        NavigationEntry{"       WORLD", Page::World, Themes::Menu_Icon::World},
        NavigationEntry{"       NETWORK", Page::Network, Themes::Menu_Icon::Network},
        NavigationEntry{"       LUA", Page::Lua, Themes::Menu_Icon::Lua},
        NavigationEntry{"       SETTINGS", Page::Settings, Themes::Menu_Icon::Settings}
    };

    for (const auto& entry : entries) {
        const bool selected = m_page == entry.page;
        const ImVec2 itemMin = ImGui::GetCursorScreenPos();
        const float itemWidth = ImGui::GetContentRegionAvail().x;
        auto* foreground = ImGui::GetForegroundDrawList();
        theme.DrawOrnateFrame(foreground, itemMin, ImVec2{itemMin.x + itemWidth, itemMin.y + 42.0F}, selected);
        theme.DrawIcon(entry.icon, foreground, ImVec2{itemMin.x + 24.0F, itemMin.y + 21.0F}, 10.5F,
            ImGui::GetColorU32(selected ? palette.emberRed : palette.bronze));
        ImGui::PushStyleColor(ImGuiCol_Button, selected ? ImVec4(0.38F, 0.055F, 0.035F, 1.00F) : palette.iron);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
