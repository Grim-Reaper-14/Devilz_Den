#include "Menu_Theme.hpp"

#include <algorithm>
#include <cmath>

namespace Devilz::Frontend::Themes
{
namespace
{
ImU32 Color(const ImVec4& color) noexcept
{
    return ImGui::GetColorU32(color);
}

ImVec2 PointOnCircle(ImVec2 center, float radius, float angle) noexcept
{
    return {center.x + std::cos(angle) * radius, center.y + std::sin(angle) * radius};
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
    style.WindowRounding = 1.0F;
    style.ChildRounding = 1.0F;
    style.FrameRounding = 1.0F;
    style.PopupRounding = 1.0F;
    style.ScrollbarRounding = 1.0F;
    style.GrabRounding = 1.0F;
    style.TabRounding = 1.0F;
    style.WindowBorderSize = 2.0F;
    style.ChildBorderSize = 1.0F;
    style.FrameBorderSize = 1.0F;
    style.WindowPadding = {13.0F, 12.0F};
    style.FramePadding = {10.0F, 7.0F};
    style.ItemSpacing = {9.0F, 7.0F};
    style.ItemInnerSpacing = {7.0F, 5.0F};

    auto& colors = style.Colors;
    colors[ImGuiCol_Text] = m_palette.parchment;
    colors[ImGuiCol_TextDisabled] = m_palette.disabledText;
    colors[ImGuiCol_WindowBg] = {0.025F, 0.018F, 0.016F, 0.985F};
    colors[ImGuiCol_ChildBg] = {0.040F, 0.030F, 0.027F, 0.970F};
    colors[ImGuiCol_PopupBg] = m_palette.stone;
    colors[ImGuiCol_Border] = m_palette.bronzeDark;
    colors[ImGuiCol_BorderShadow] = {0.0F, 0.0F, 0.0F, 0.82F};
    colors[ImGuiCol_FrameBg] = {0.070F, 0.055F, 0.048F, 1.00F};
    colors[ImGuiCol_FrameBgHovered] = {0.15F, 0.070F, 0.052F, 1.00F};
    colors[ImGuiCol_FrameBgActive] = {0.31F, 0.055F, 0.035F, 1.00F};
    colors[ImGuiCol_TitleBg] = m_palette.deepStone;
    colors[ImGuiCol_TitleBgActive] = m_palette.stone;
    colors[ImGuiCol_Button] = {0.075F, 0.055F, 0.047F, 1.00F};
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

void Menu_Theme_Manager::DrawOrnateFrame(
    ImDrawList* draw,
    ImVec2 min,
    ImVec2 max,
    bool active) const noexcept
{
    if (!draw)
        return;

    const ImU32 iron = Color(m_palette.ironLight);
    const ImU32 bronze = Color(active ? m_palette.emberRed : m_palette.bronzeDark);
    const ImU32 inner = Color(active ? m_palette.emberGlow : ImVec4{0.20F, 0.14F, 0.09F, 0.72F});

    draw->AddRect(min, max, iron, 1.0F, 0, 3.0F);
    draw->AddRect({min.x + 4.0F, min.y + 4.0F}, {max.x - 4.0F, max.y - 4.0F}, bronze, 1.0F, 0, active ? 2.0F : 1.0F);
    draw->AddRect({min.x + 8.0F, min.y + 8.0F}, {max.x - 8.0F, max.y - 8.0F}, inner, 1.0F, 0, 1.0F);

    constexpr float corner = 13.0F;
    const ImU32 ornament = Color(active ? m_palette.emberRed : m_palette.bronze);
    draw->AddLine({min.x, min.y + corner}, {min.x + corner, min.y}, ornament, 1.5F);
    draw->AddLine({max.x - corner, min.y}, {max.x, min.y + corner}, ornament, 1.5F);
    draw->AddLine({min.x, max.y - corner}, {min.x + corner, max.y}, ornament, 1.5F);
    draw->AddLine({max.x - corner, max.y}, {max.x, max.y - corner}, ornament, 1.5F);

    const ImVec2 top{(min.x + max.x) * 0.5F, min.y + 4.0F};
    draw->AddQuadFilled(
        {top.x, top.y - 4.0F}, {top.x + 4.0F, top.y},
        {top.x, top.y + 4.0F}, {top.x - 4.0F, top.y}, ornament);
}

void Menu_Theme_Manager::DrawIcon(
    Menu_Icon icon,
    ImDrawList* draw,
    ImVec2 center,
    float radius,
    ImU32 color) const noexcept
{
    if (!draw || radius <= 0.0F)
        return;

    const ImU32 stroke = color != 0 ? color : Color(m_palette.bronze);
    const ImU32 dark = Color(ImVec4{0.02F, 0.015F, 0.012F, 0.96F});
    const ImU32 red = Color(m_palette.emberRed);
    const float t = (std::max)(1.25F, radius * 0.105F);

    switch (icon) {
    case Menu_Icon::DevilCrest: {
        draw->AddCircleFilled(center, radius * 0.70F, dark, 20);
        draw->AddCircle(center, radius * 0.70F, stroke, 20, t);
        draw->AddTriangleFilled(
            {center.x - radius * 0.55F, center.y - radius * 0.30F},
            {center.x - radius * 1.05F, center.y - radius * 0.90F},
            {center.x - radius * 0.18F, center.y - radius * 0.62F}, dark);
        draw->AddTriangle(
            {center.x - radius * 0.55F, center.y - radius * 0.30F},
            {center.x - radius * 1.05F, center.y - radius * 0.90F},
            {center.x - radius * 0.18F, center.y - radius * 0.62F}, stroke, t);
        draw->AddTriangleFilled(
            {center.x + radius * 0.55F, center.y - radius * 0.30F},
            {center.x + radius * 1.05F, center.y - radius * 0.90F},
            {center.x + radius * 0.18F, center.y - radius * 0.62F}, dark);
        draw->AddTriangle(
            {center.x + radius * 0.55F, center.y - radius * 0.30F},
            {center.x + radius * 1.05F, center.y - radius * 0.90F},
            {center.x + radius * 0.18F, center.y - radius * 0.62F}, stroke, t);
        draw->AddCircleFilled({center.x - radius * 0.25F, center.y - radius * 0.05F}, radius * 0.10F, red, 8);
        draw->AddCircleFilled({center.x + radius * 0.25F, center.y - radius * 0.05F}, radius * 0.10F, red, 8);
        draw->AddTriangleFilled(
            {center.x, center.y + radius * 0.10F},
            {center.x - radius * 0.14F, center.y + radius * 0.43F},
            {center.x + radius * 0.14F, center.y + radius * 0.43F}, stroke);
        break;
    }
    case Menu_Icon::Self: {
        draw->PathClear();
        draw->PathLineTo({center.x, center.y - radius});
        draw->PathLineTo({center.x + radius * 0.72F, center.y - radius * 0.52F});
        draw->PathLineTo({center.x + radius * 0.56F, center.y + radius * 0.52F});
        draw->PathLineTo({center.x, center.y + radius});
        draw->PathLineTo({center.x - radius * 0.56F, center.y + radius * 0.52F});
        draw->PathLineTo({center.x - radius * 0.72F, center.y - radius * 0.52F});
        draw->PathStroke(stroke, ImDrawFlags_Closed, t);
        draw->AddLine({center.x, center.y - radius * 0.66F}, {center.x, center.y + radius * 0.56F}, stroke, t);
        break;
    }
    case Menu_Icon::Weapons: {
        draw->AddLine({center.x - radius * 0.72F, center.y + radius * 0.72F}, {center.x + radius * 0.72F, center.y - radius * 0.72F}, stroke, t);
        draw->AddLine({center.x + radius * 0.72F, center.y + radius * 0.72F}, {center.x - radius * 0.72F, center.y - radius * 0.72F}, stroke, t);
        draw->AddTriangleFilled({center.x + radius * 0.72F, center.y - radius * 0.72F}, {center.x + radius * 0.34F, center.y - radius * 0.59F}, {center.x + radius * 0.59F, center.y - radius * 0.34F}, stroke);
        draw->AddTriangleFilled({center.x - radius * 0.72F, center.y - radius * 0.72F}, {center.x - radius * 0.34F, center.y - radius * 0.59F}, {center.x - radius * 0.59F, center.y - radius * 0.34F}, stroke);
        break;
    }
    case Menu_Icon::Vehicle: {
        draw->AddCircle(center, radius * 0.86F, stroke, 20, t);
        draw->AddCircle(center, radius * 0.27F, stroke, 16, t);
        for (int i = 0; i < 8; ++i) {
            const float a = static_cast<float>(i) * 0.785398163F;
            draw->AddLine(PointOnCircle(center, radius * 0.31F, a), PointOnCircle(center, radius * 0.78F, a), stroke, t * 0.75F);
        }
        break;
    }
    case Menu_Icon::Teleport: {
        draw->AddCircle(center, radius * 0.86F, stroke, 20, t);
        draw->AddTriangleFilled({center.x, center.y - radius}, {center.x - radius * 0.18F, center.y}, {center.x + radius * 0.18F, center.y}, red);
        draw->AddTriangleFilled({center.x, center.y + radius}, {center.x - radius * 0.18F, center.y}, {center.x + radius * 0.18F, center.y}, stroke);
        draw->AddCircleFilled(center, radius * 0.11F, stroke, 8);
        break;
    }
    case Menu_Icon::World: {
        draw->AddCircle(center, radius * 0.86F, stroke, 20, t);
        draw->AddLine({center.x - radius * 0.82F, center.y}, {center.x + radius * 0.82F, center.y}, stroke, t * 0.8F);
        draw->AddLine({center.x, center.y - radius * 0.82F}, {center.x, center.y + radius * 0.82F}, stroke, t * 0.8F);
        draw->AddCircle(center, radius * 0.48F, stroke, 20, t * 0.65F);
        break;
    }
    case Menu_Icon::Network: {
        const ImVec2 a{center.x, center.y - radius * 0.70F};
        const ImVec2 b{center.x - radius * 0.68F, center.y + radius * 0.50F};
        const ImVec2 c{center.x + radius * 0.68F, center.y + radius * 0.50F};
        draw->AddLine(a, b, stroke, t);
        draw->AddLine(b, c, stroke, t);
        draw->AddLine(c, a, stroke, t);
        draw->AddCircleFilled(a, radius * 0.18F, dark, 10); draw->AddCircle(a, radius * 0.18F, stroke, 10, t);
        draw->AddCircleFilled(b, radius * 0.18F, dark, 10); draw->AddCircle(b, radius * 0.18F, stroke, 10, t);
        draw->AddCircleFilled(c, radius * 0.18F, dark, 10); draw->AddCircle(c, radius * 0.18F, stroke, 10, t);
        break;
    }
    case Menu_Icon::Lua: {
        const ImVec2 min{center.x - radius * 0.64F, center.y - radius * 0.76F};
        const ImVec2 max{center.x + radius * 0.64F, center.y + radius * 0.76F};
        draw->AddRect(min, max, stroke, radius * 0.12F, 0, t);
        draw->AddCircle({min.x, center.y - radius * 0.62F}, radius * 0.16F, stroke, 10, t);
        draw->AddCircle({max.x, center.y + radius * 0.62F}, radius * 0.16F, stroke, 10, t);
        draw->AddLine({min.x + radius * 0.22F, center.y - radius * 0.22F}, {max.x - radius * 0.20F, center.y - radius * 0.22F}, stroke, t * 0.70F);
        draw->AddLine({min.x + radius * 0.22F, center.y + radius * 0.10F}, {max.x - radius * 0.30F, center.y + radius * 0.10F}, stroke, t * 0.70F);
        break;
    }
    case Menu_Icon::Settings: {
        draw->AddCircle(center, radius * 0.56F, stroke, 20, t);
        draw->AddCircle(center, radius * 0.18F, stroke, 12, t);
        for (int i = 0; i < 8; ++i) {
            const float a = static_cast<float>(i) * 0.785398163F;
            draw->AddLine(PointOnCircle(center, radius * 0.60F, a), PointOnCircle(center, radius * 0.94F, a), stroke, t * 1.2F);
        }
        break;
    }
    }
}

void Menu_Theme_Manager::DrawHeader(const char* title, const char* subtitle) const noexcept
{
    const auto origin = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;
    constexpr float height = 122.0F;
    auto* draw = ImGui::GetWindowDrawList();
    const ImVec2 max{origin.x + width, origin.y + height};

    draw->AddRectFilled(origin, max, Color(ImVec4{0.025F, 0.017F, 0.014F, 1.0F}), 1.0F);
    constexpr float blockHeight = 24.0F;
    for (int row = 0; row < 5; ++row) {
        const float y0 = origin.y + static_cast<float>(row) * blockHeight;
        const float y1 = (std::min)(max.y, y0 + blockHeight);
        const float offset = (row & 1) ? 35.0F : 0.0F;
        for (float x = origin.x - offset; x < max.x; x += 70.0F) {
            const ImVec2 a{(std::max)(x, origin.x), y0};
            const ImVec2 b{(std::min)(x + 68.0F, max.x), y1};
            draw->AddRectFilled(a, b, Color((row & 1) ? m_palette.stone : m_palette.deepStone));
            draw->AddRect(a, b, Color(ImVec4{0.20F, 0.13F, 0.10F, 0.34F}), 0.0F, 0, 1.0F);
        }
    }

    DrawOrnateFrame(draw, origin, max, true);

    const float plaqueWidth = (std::min)(width * 0.64F, 720.0F);
    const ImVec2 plaqueMin{origin.x + (width - plaqueWidth) * 0.5F, origin.y + 13.0F};
    const ImVec2 plaqueMax{plaqueMin.x + plaqueWidth, origin.y + 88.0F};
    draw->AddRectFilled(plaqueMin, plaqueMax, Color(ImVec4{0.055F, 0.018F, 0.015F, 0.96F}), 1.0F);
    DrawOrnateFrame(draw, plaqueMin, plaqueMax, true);
    draw->AddLine({plaqueMin.x + 18.0F, plaqueMin.y + 10.0F}, {plaqueMax.x - 18.0F, plaqueMin.y + 10.0F}, Color(m_palette.emberGlow), 2.0F);
    draw->AddLine({plaqueMin.x + 18.0F, plaqueMax.y - 10.0F}, {plaqueMax.x - 18.0F, plaqueMax.y - 10.0F}, Color(m_palette.emberGlow), 2.0F);

    const char* safeTitle = title ? title : "DEVIL'S DEN MENU";
    const auto titleSize = ImGui::CalcTextSize(safeTitle);
    const ImVec2 titlePos{origin.x + (width - titleSize.x) * 0.5F, origin.y + 29.0F};
    draw->AddText({titlePos.x + 3.0F, titlePos.y + 3.0F}, Color(ImVec4{0.0F, 0.0F, 0.0F, 0.98F}), safeTitle);
    draw->AddText({titlePos.x + 1.0F, titlePos.y + 1.0F}, Color(m_palette.bronzeDark), safeTitle);
    draw->AddText(titlePos, Color(m_palette.emberRed), safeTitle);

    if (subtitle && *subtitle) {
        const auto subtitleSize = ImGui::CalcTextSize(subtitle);
        const ImVec2 subtitlePos{origin.x + (width - subtitleSize.x) * 0.5F, origin.y + 57.0F};
        draw->AddText(subtitlePos, Color(m_palette.bronze), subtitle);
    }

    const ImVec2 crestCenter{origin.x + width * 0.5F, origin.y + 101.0F};
    draw->AddCircleFilled(crestCenter, 18.0F, Color(ImVec4{0.035F, 0.015F, 0.012F, 1.0F}), 24);
    draw->AddCircle(crestCenter, 19.0F, Color(m_palette.emberGlow), 24, 3.0F);
    DrawIcon(Menu_Icon::DevilCrest, draw, crestCenter, 12.0F, Color(m_palette.bronze));

    // Side spear/finial details inspired by the reference's gothic frame.
    for (const float side : {origin.x + 34.0F, max.x - 34.0F}) {
        draw->AddLine({side, origin.y + 14.0F}, {side, max.y - 14.0F}, Color(m_palette.ironLight), 3.0F);
        draw->AddTriangleFilled({side, origin.y + 4.0F}, {side - 5.0F, origin.y + 16.0F}, {side + 5.0F, origin.y + 16.0F}, Color(m_palette.bronzeDark));
        draw->AddCircleFilled({side, max.y - 15.0F}, 3.0F, Color(m_palette.emberRed), 8);
    }

    ImGui::Dummy(ImVec2{width, height});
}

void Menu_Theme_Manager::DrawDivider() const noexcept
{
    const auto start = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;
    auto* draw = ImGui::GetWindowDrawList();
    const float y = start.y + 5.0F;

    draw->AddLine({start.x, y}, {start.x + width, y}, Color(m_palette.bronzeDark), 1.0F);
    draw->AddLine({start.x + width * 0.20F, y}, {start.x + width * 0.80F, y}, Color(m_palette.emberGlow), 2.0F);
    draw->AddQuadFilled(
        {start.x + width * 0.5F, y - 5.0F}, {start.x + width * 0.5F + 5.0F, y},
        {start.x + width * 0.5F, y + 5.0F}, {start.x + width * 0.5F - 5.0F, y}, Color(m_palette.emberRed));
    ImGui::Dummy(ImVec2{0.0F, 13.0F});
}
}
