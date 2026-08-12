#pragma once

#include <imgui.h>

#include <cstdint>

namespace Devilz::Frontend::Themes
{
enum class Menu_Theme_Id : std::uint8_t
{
    DevilsDenMedieval = 0,
    BloodIron = 1,
    AshenGold = 2
};

inline constexpr int Menu_Theme_Count = 3;

[[nodiscard]] inline const char* MenuThemeName(Menu_Theme_Id theme) noexcept
{
    switch (theme) {
    case Menu_Theme_Id::BloodIron: return "Blood & Iron";
    case Menu_Theme_Id::AshenGold: return "Ashen Gold";
    case Menu_Theme_Id::DevilsDenMedieval:
    default: return "Devil's Den Medieval";
    }
}

enum class Menu_Icon : std::uint8_t
{
    DevilCrest,
    Self,
    Weapons,
    Vehicle,
    Teleport,
    World,
    Network,
    Lua,
    Settings
};

struct Menu_Theme_Palette
{
    ImVec4 emberRed{};
    ImVec4 emberGlow{};
    ImVec4 bronze{};
    ImVec4 bronzeDark{};
    ImVec4 iron{};
    ImVec4 ironLight{};
    ImVec4 deepStone{};
    ImVec4 stone{};
    ImVec4 stoneLight{};
    ImVec4 parchment{};
    ImVec4 disabledText{};
};

class Menu_Theme_Manager final
{
public:
    static Menu_Theme_Manager& Instance() noexcept;

    void SetTheme(Menu_Theme_Id theme) noexcept;
    void ApplyPreset(Menu_Theme_Id theme) noexcept
    {
        m_theme = theme;
        switch (theme) {
        case Menu_Theme_Id::BloodIron:
            m_palette.emberRed = {0.96F, 0.035F, 0.025F, 1.00F};
            m_palette.emberGlow = {0.72F, 0.020F, 0.015F, 0.82F};
            m_palette.bronze = {0.68F, 0.52F, 0.36F, 1.00F};
            m_palette.bronzeDark = {0.28F, 0.13F, 0.10F, 1.00F};
            m_palette.iron = {0.085F, 0.070F, 0.070F, 1.00F};
            m_palette.ironLight = {0.24F, 0.20F, 0.20F, 1.00F};
            m_palette.deepStone = {0.030F, 0.022F, 0.024F, 1.00F};
            m_palette.stone = {0.075F, 0.050F, 0.052F, 1.00F};
            m_palette.stoneLight = {0.15F, 0.105F, 0.105F, 1.00F};
            m_palette.parchment = {0.88F, 0.80F, 0.73F, 1.00F};
            m_palette.disabledText = {0.47F, 0.39F, 0.38F, 1.00F};
            break;
        case Menu_Theme_Id::AshenGold:
            m_palette.emberRed = {0.92F, 0.34F, 0.055F, 1.00F};
            m_palette.emberGlow = {0.72F, 0.31F, 0.045F, 0.74F};
            m_palette.bronze = {0.86F, 0.72F, 0.38F, 1.00F};
            m_palette.bronzeDark = {0.39F, 0.29F, 0.12F, 1.00F};
            m_palette.iron = {0.12F, 0.115F, 0.10F, 1.00F};
            m_palette.ironLight = {0.32F, 0.30F, 0.25F, 1.00F};
            m_palette.deepStone = {0.050F, 0.048F, 0.041F, 1.00F};
            m_palette.stone = {0.10F, 0.095F, 0.075F, 1.00F};
            m_palette.stoneLight = {0.19F, 0.18F, 0.14F, 1.00F};
            m_palette.parchment = {0.94F, 0.88F, 0.69F, 1.00F};
            m_palette.disabledText = {0.51F, 0.48F, 0.38F, 1.00F};
            break;
        case Menu_Theme_Id::DevilsDenMedieval:
        default:
            m_palette.emberRed = {0.88F, 0.10F, 0.045F, 1.00F};
            m_palette.emberGlow = {0.62F, 0.035F, 0.018F, 0.75F};
            m_palette.bronze = {0.78F, 0.66F, 0.44F, 1.00F};
            m_palette.bronzeDark = {0.34F, 0.24F, 0.13F, 1.00F};
            m_palette.iron = {0.13F, 0.11F, 0.10F, 1.00F};
            m_palette.ironLight = {0.27F, 0.23F, 0.20F, 1.00F};
            m_palette.deepStone = {0.055F, 0.045F, 0.040F, 1.00F};
            m_palette.stone = {0.095F, 0.080F, 0.072F, 1.00F};
            m_palette.stoneLight = {0.17F, 0.145F, 0.125F, 1.00F};
            m_palette.parchment = {0.84F, 0.78F, 0.64F, 1.00F};
            m_palette.disabledText = {0.47F, 0.42F, 0.36F, 1.00F};
            break;
        }
    }

    [[nodiscard]] Menu_Theme_Id CurrentTheme() const noexcept { return m_theme; }
    [[nodiscard]] const Menu_Theme_Palette& Palette() const noexcept { return m_palette; }

    void Apply() const noexcept;
    void DrawHeader(const char* title, const char* subtitle) const noexcept;
    void DrawDivider() const noexcept;
    void DrawIcon(Menu_Icon icon, ImDrawList* draw, ImVec2 center, float radius, ImU32 color = 0) const noexcept;
    void DrawOrnateFrame(ImDrawList* draw, ImVec2 min, ImVec2 max, bool active = false) const noexcept;

private:
    Menu_Theme_Manager() noexcept;
    void LoadPalette(Menu_Theme_Id theme) noexcept;

    Menu_Theme_Id m_theme = Menu_Theme_Id::DevilsDenMedieval;
    Menu_Theme_Palette m_palette{};
};
}
