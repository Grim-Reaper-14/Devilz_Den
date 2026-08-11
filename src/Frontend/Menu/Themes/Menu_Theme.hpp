#pragma once

#include <imgui.h>

#include <cstdint>

namespace Devilz::Frontend::Themes
{
enum class Menu_Theme_Id : std::uint8_t
{
    DevilsDenMedieval
};

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
