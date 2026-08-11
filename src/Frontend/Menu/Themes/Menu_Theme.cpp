#include "Menu_Theme.hpp"

#include <algorithm>

namespace Devilz::Frontend::Themes
{
namespace
{
ImU32 Color(const ImVec4& color) noexcept
{
    return ImGui::GetColorU32(color);
}
}

Menu_Theme_Manager& Menu_Theme_Manager::Instance() noexcept
{
    static Menu_Theme_Manager manager;
    return manager;
}

Menu_Theme_Manager::Menu_Theme_Manager() noexcept
{
    LoadPalette(m_theme);
}

void Menu_Theme_Manager::SetTheme(Menu_Theme_Id theme) noexcept
{
    m_theme = theme;
    LoadPalette(theme);
}

void Menu_Theme_Manager::LoadPalette(Menu_Theme_Id theme) noexcept
{
    switch (theme) {
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

void Menu_Theme_Manager::Apply() const noexcept
{
    auto& style = ImGui::GetStyle();
    style.WindowRounding = 2.0F;
    style.ChildRounding = 2.0F;
    style.FrameRounding = 1.0F;
    style.PopupRounding = 2.0F;
    style.ScrollbarRounding = 1.0F;
    style.GrabRounding = 1.0F;
    style.TabRounding = 1.0F;
    style.WindowBorderSize = 1.0F;
    style.ChildBorderSize = 1.0F;
    style.FrameBorderSize = 1.0F;
    style.WindowPadding = {12.0F, 12.0F};
    style.FramePadding = {9.0F, 6.0F};
    style.ItemSpacing = {9.0F, 7.0F};

    auto& colors = style.Colors;
    colors[ImGuiCol_Text] = m_palette.parchment;
    colors[ImGuiCol_TextDisabled] = m_palette.disabledText;
    colors[ImGuiCol_WindowBg] = m_palette.deepStone;
    colors[ImGuiCol_ChildBg] = m_palette.deepStone;
    colors[ImGuiCol_PopupBg] = m_palette.stone;
    colors[ImGuiCol_Border] = m_palette.bronzeDark;
    colors[ImGuiCol_BorderShadow] = {0.0F, 0.0F, 0.0F, 0.72F};
    colors[ImGuiCol_FrameBg] = m_palette.iron;
    colors[ImGuiCol_FrameBgHovered] = m_palette.ironLight;
    colors[ImGuiCol_FrameBgActive] = {0.31F, 0.055F, 0.035F, 1.00F};
    colors[ImGuiCol_TitleBg] = m_palette.deepStone;
    colors[ImGuiCol_TitleBgActive] = m_palette.stone;
    colors[ImGuiCol_Button] = m_palette.iron;
    colors[ImGuiCol_ButtonHovered] = {0.31F, 0.055F, 0.035F, 1.00F};
    colors[ImGuiCol_ButtonActive] = {0.47F, 0.075F, 0.040F, 1.00F};
    colors[ImGuiCol_Header] = {0.30F, 0.050F, 0.033F, 1.00F};
    colors[ImGuiCol_HeaderHovered] = {0.44F, 0.070F, 0.040F, 1.00F};
    colors[ImGuiCol_HeaderActive] = {0.58F, 0.085F, 0.044F, 1.00F};
    colors[ImGuiCol_CheckMark] = m_palette.emberRed;
    colors[ImGuiCol_SliderGrab] = m_palette.bronze;
    colors[ImGuiCol_SliderGrabActive] = m_palette.emberRed;
    colors[ImGuiCol_Tab] = m_palette.iron;
    colors[ImGuiCol_TabHovered] = {0.40F, 0.060F, 0.038F, 1.00F};
    colors[ImGuiCol_TabSelected] = {0.34F, 0.050F, 0.032F, 1.00F};
    colors[ImGuiCol_Separator] = m_palette.bronzeDark;
}

void Menu_Theme_Manager::DrawHeader(const char* title, const char* subtitle) const noexcept
{
    const auto origin = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;
    constexpr float height = 108.0F;
    auto* draw = ImGui::GetWindowDrawList();
    const ImVec2 max{origin.x + width, origin.y + height};

    // Carved black-stone slab.
    draw->AddRectFilled(origin, max, Color(m_palette.deepStone), 2.0F);
    constexpr float blockHeight = 22.0F;
    for (int row = 0; row < 5; ++row) {
        const float y0 = origin.y + static_cast<float>(row) * blockHeight;
        const float y1 = (std::min)(max.y, y0 + blockHeight);
        const float offset = (row & 1) ? 30.0F : 0.0F;
        for (float x = origin.x - offset; x < max.x; x += 60.0F) {
            const ImVec2 a{(std::max)(x, origin.x), y0};
            const ImVec2 b{(std::min)(x + 58.0F, max.x), y1};
            draw->AddRectFilled(a, b, Color((row & 1) ? m_palette.stone : m_palette.deepStone));
            draw->AddRect(a, b, Color(ImVec4{0.19F, 0.16F, 0.14F, 0.38F}), 0.0F, 0, 1.0F);
        }
    }

    // Blackened iron outer bands and aged bronze inner trim.
    draw->AddRect(origin, max, Color(m_palette.ironLight), 2.0F, 0, 5.0F);
    draw->AddRect(ImVec2{origin.x + 5.0F, origin.y + 5.0F}, ImVec2{max.x - 5.0F, max.y - 5.0F},
        Color(m_palette.bronzeDark), 1.0F, 0, 2.0F);
    draw->AddRect(ImVec2{origin.x + 10.0F, origin.y + 10.0F}, ImVec2{max.x - 10.0F, max.y - 10.0F},
        Color(ImVec4{m_palette.emberGlow.x, m_palette.emberGlow.y, m_palette.emberGlow.z, 0.55F}), 1.0F, 0, 1.0F);

    // Forged rivets.
    constexpr float rivetRadius = 3.5F;
    const ImU32 rivet = Color(m_palette.bronze);
    const ImU32 rivetShadow = Color(ImVec4{0.02F, 0.015F, 0.012F, 0.95F});
    const ImVec2 rivets[]{
        {origin.x + 16.0F, origin.y + 16.0F}, {max.x - 16.0F, origin.y + 16.0F},
        {origin.x + 16.0F, max.y - 16.0F}, {max.x - 16.0F, max.y - 16.0F}
    };
    for (const auto& p : rivets) {
        draw->AddCircleFilled(ImVec2{p.x + 1.5F, p.y + 1.5F}, rivetRadius + 0.5F, rivetShadow);
        draw->AddCircleFilled(p, rivetRadius, rivet);
        draw->AddCircle(p, rivetRadius, Color(m_palette.bronzeDark), 0, 1.0F);
    }

    // Central medieval plaque behind the title.
    const float plaqueWidth = (std::min)(width * 0.58F, 560.0F);
    const ImVec2 plaqueMin{origin.x + (width - plaqueWidth) * 0.5F, origin.y + 20.0F};
    const ImVec2 plaqueMax{plaqueMin.x + plaqueWidth, origin.y + 82.0F};
    draw->AddRectFilled(plaqueMin, plaqueMax, Color(ImVec4{0.07F, 0.025F, 0.020F, 0.96F}), 1.0F);
    draw->AddRect(plaqueMin, plaqueMax, Color(m_palette.bronze), 1.0F, 0, 2.0F);
    draw->AddLine(ImVec2{plaqueMin.x + 12.0F, plaqueMin.y + 9.0F}, ImVec2{plaqueMax.x - 12.0F, plaqueMin.y + 9.0F}, Color(m_palette.emberGlow), 2.0F);
    draw->AddLine(ImVec2{plaqueMin.x + 12.0F, plaqueMax.y - 9.0F}, ImVec2{plaqueMax.x - 12.0F, plaqueMax.y - 9.0F}, Color(m_palette.emberGlow), 2.0F);

    const char* safeTitle = title ? title : "DEVIL'S DEN";
    const auto titleSize = ImGui::CalcTextSize(safeTitle);
    const ImVec2 titlePos{origin.x + (width - titleSize.x) * 0.5F, origin.y + 31.0F};
    draw->AddText(ImVec2{titlePos.x + 2.0F, titlePos.y + 2.0F}, Color(ImVec4{0.0F, 0.0F, 0.0F, 0.95F}), safeTitle);
    draw->AddText(titlePos, Color(m_palette.emberRed), safeTitle);

    if (subtitle && *subtitle) {
        const auto subtitleSize = ImGui::CalcTextSize(subtitle);
        const ImVec2 subtitlePos{origin.x + (width - subtitleSize.x) * 0.5F, origin.y + 58.0F};
        draw->AddText(subtitlePos, Color(m_palette.bronze), subtitle);
    }

    ImGui::Dummy(ImVec2{width, height});
}

void Menu_Theme_Manager::DrawDivider() const noexcept
{
    const auto start = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;
    auto* draw = ImGui::GetWindowDrawList();
    const float y = start.y + 5.0F;

    draw->AddLine(ImVec2{start.x, y}, ImVec2{start.x + width, y}, Color(m_palette.bronzeDark), 1.0F);
    draw->AddLine(ImVec2{start.x + width * 0.20F, y}, ImVec2{start.x + width * 0.80F, y}, Color(m_palette.emberGlow), 2.0F);
    draw->AddCircleFilled(ImVec2{start.x + width * 0.5F, y}, 4.0F, Color(m_palette.emberRed));
    draw->AddCircle(ImVec2{start.x + width * 0.5F, y}, 6.0F, Color(m_palette.bronzeDark), 0, 1.0F);
    ImGui::Dummy(ImVec2{0.0F, 13.0F});
}
}
