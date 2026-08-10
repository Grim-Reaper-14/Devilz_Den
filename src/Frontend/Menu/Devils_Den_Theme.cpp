#include "Devils_Den_Theme.hpp"

#include <imgui.h>

namespace Devilz::Frontend
{
void Devils_Den_Theme::Apply() noexcept
{
    auto& style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(14.0F, 14.0F);
    style.FramePadding = ImVec2(10.0F, 7.0F);
    style.ItemSpacing = ImVec2(10.0F, 8.0F);
    style.ItemInnerSpacing = ImVec2(8.0F, 6.0F);
    style.WindowRounding = 2.0F;
    style.ChildRounding = 2.0F;
    style.FrameRounding = 2.0F;
    style.PopupRounding = 2.0F;
    style.ScrollbarRounding = 2.0F;
    style.GrabRounding = 2.0F;
    style.WindowBorderSize = 2.0F;
    style.ChildBorderSize = 1.0F;
    style.FrameBorderSize = 1.0F;

    auto* colors = style.Colors;
    colors[ImGuiCol_Text] = ImVec4(0.90F, 0.85F, 0.72F, 1.00F);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.45F, 0.41F, 0.36F, 1.00F);
    colors[ImGuiCol_WindowBg] = ImVec4(0.035F, 0.030F, 0.028F, 0.98F);
    colors[ImGuiCol_ChildBg] = ImVec4(0.055F, 0.045F, 0.040F, 0.96F);
    colors[ImGuiCol_PopupBg] = ImVec4(0.045F, 0.035F, 0.032F, 0.98F);
    colors[ImGuiCol_Border] = ImVec4(0.34F, 0.08F, 0.055F, 0.95F);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.0F, 0.0F, 0.0F, 0.0F);
    colors[ImGuiCol_FrameBg] = ImVec4(0.10F, 0.075F, 0.065F, 1.00F);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24F, 0.055F, 0.040F, 1.00F);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.34F, 0.045F, 0.032F, 1.00F);
    colors[ImGuiCol_TitleBg] = ImVec4(0.07F, 0.025F, 0.020F, 1.00F);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.20F, 0.035F, 0.025F, 1.00F);
    colors[ImGuiCol_CheckMark] = ImVec4(0.92F, 0.12F, 0.055F, 1.00F);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.62F, 0.10F, 0.055F, 1.00F);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.95F, 0.16F, 0.07F, 1.00F);
    colors[ImGuiCol_Button] = ImVec4(0.15F, 0.050F, 0.038F, 1.00F);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.34F, 0.065F, 0.042F, 1.00F);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.48F, 0.075F, 0.045F, 1.00F);
    colors[ImGuiCol_Header] = ImVec4(0.25F, 0.045F, 0.032F, 1.00F);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.38F, 0.060F, 0.040F, 1.00F);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.52F, 0.075F, 0.045F, 1.00F);
    colors[ImGuiCol_Separator] = ImVec4(0.32F, 0.080F, 0.050F, 0.90F);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.62F, 0.11F, 0.060F, 1.00F);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.90F, 0.15F, 0.070F, 1.00F);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.025F, 0.020F, 0.018F, 1.00F);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.18F, 0.060F, 0.045F, 1.00F);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.34F, 0.075F, 0.050F, 1.00F);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.48F, 0.09F, 0.055F, 1.00F);
}
}
