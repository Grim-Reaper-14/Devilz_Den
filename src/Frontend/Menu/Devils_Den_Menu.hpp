#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace Devilz::Frontend
{
class Devils_Den_Menu final
{
public:
    void Draw(bool& open);

private:
    enum class Page : std::size_t
    {
        Self,
        Weapons,
        Vehicle,
        Teleport,
        World,
        Network,
        Unlocks,
        Lua,
        Settings
    };

    void DrawLegacy(bool& open);
    void DrawBanner();
    void DrawBannerLegacy();
    void DrawNavigation();
    void DrawNavigationLegacy();
    void DrawSelfPage();
    void DrawSelfPageLegacy();
    void DrawWeaponsPage();
    void DrawVehiclePage();
    void DrawTeleportPage();
    void DrawTeleportPageLegacy();
    void DrawWorldPage();
    void DrawSettingsPage();
    void DrawSettingsPageLegacy();
    void DrawPlaceholderPage(const char* title, const char* detail);
    void DrawPlaceholderPageLegacy(const char* title, const char* detail);

    Page m_page = Page::Self;
    bool m_godMode = false;
    bool m_neverWanted = false;
    bool m_superJump = false;
    bool m_infiniteOxygen = false;
    bool m_noRagdoll = false;
    bool m_keepPlayerClean = false;
    bool m_infiniteAmmo = false;
    bool m_fastRun = false;
    float m_health = 100.0F;

    std::array<char, 64> m_vehicleSearch{};
    int m_vehicleClassFilter = -1;
    std::uint32_t m_selectedVehicleModel = 0;
    bool m_spawnInsideVehicle = true;
    bool m_spawnVehicleMaxed = false;
    bool m_spawnVehicleOnGround = true;
    bool m_spawnVehicleEngineRunning = true;
    bool m_spawnVehicleInvincible = false;
    bool m_spawnVehicleClean = true;

    int m_forgeModSlot = 11;
    int m_forgeModIndex = -1;
    int m_forgeWheelType = 7;
    int m_forgeWheelIndex = -1;
    bool m_forgeCustomTires = false;
    int m_forgeWindowTint = 0;
    int m_forgePlateStyle = 0;
    std::array<char, 9> m_forgePlateText{};
    int m_forgePrimaryPaintType = 1;
    int m_forgePrimaryColor = 0;
    int m_forgeSecondaryPaintType = 1;
    int m_forgeSecondaryColor = 0;
    int m_forgePearlescent = 0;
    int m_forgeWheelColor = 0;
    bool m_forgeLoweredStance = false;
    std::array<int, 3> m_forgePrimaryRgb{80, 0, 0};
    std::array<int, 3> m_forgeSecondaryRgb{0, 0, 0};
};
}
