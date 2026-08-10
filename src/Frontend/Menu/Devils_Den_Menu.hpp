#pragma once

#include <array>
#include <cstddef>

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
        Settings
    };

    void DrawBanner();
    void DrawNavigation();
    void DrawSelfPage();
    void DrawPlaceholderPage(const char* title, const char* detail);

    Page m_page = Page::Self;
    bool m_godMode = false;
    bool m_neverWanted = false;
    bool m_fastRun = false;
    bool m_superJump = false;
    float m_health = 100.0F;
};
}
