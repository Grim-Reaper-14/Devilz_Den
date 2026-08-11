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

    Themes::Menu_Theme_Manager::Instance().Apply();

    ImGui::SetNextWindowSize(ImVec2(980.0F, 680.0F), ImGuiCond_FirstUseEver);
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
    const auto& palette = Themes::Menu_Theme_Manager::Instance().Palette();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, palette.deepStone);
    ImGui::TextColored(palette.bronze, "CATEGORIES");
    MedievalDivider();

    struct NavigationEntry { const char* label; Page page; };
    constexpr std::array entries{
        NavigationEntry{"SELF", Page::Self}, NavigationEntry{"WEAPONS", Page::Weapons},
        NavigationEntry{"VEHICLE", Page::Vehicle}, NavigationEntry{"TELEPORT", Page::Teleport},
        NavigationEntry{"WORLD", Page::World}, NavigationEntry{"NETWORK", Page::Network},
        NavigationEntry{"LUA", Page::Lua}, NavigationEntry{"SETTINGS", Page::Settings}
    };

    for (const auto& entry : entries) {
        const bool selected = m_page == entry.page;
        ImGui::PushStyleColor(ImGuiCol_Button, selected ? ImVec4(0.38F, 0.055F, 0.035F, 1.00F) : palette.iron);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
