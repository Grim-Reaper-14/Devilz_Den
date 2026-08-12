#include "Frontend/Menu/Menu_Appearance.hpp"

namespace Devilz::Frontend
{
void Devils_Den_Menu::DrawBanner()
{
    auto& appearance = Menu_Appearance_State::Instance();
    appearance.InitializeOnce();

    auto& image = Renderer::D3D12_Image_Loader::Instance();
    image.Tick();

    if (!appearance.BannerEnabled() || !appearance.BannerLoaded()) {
        DrawBannerLegacy();
        return;
    }

    auto& theme = Themes::Menu_Theme_Manager::Instance();
    const auto& palette = theme.Palette();

    ImVec2 origin = ImGui::GetCursorScreenPos();
    float width = (std::max)(1.0F, ImGui::GetContentRegionAvail().x);
    constexpr float height = 122.0F;
    const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    constexpr float viewportMargin = 8.0F;
    if (origin.y < viewportMargin) {
        origin.y = viewportMargin;
        ImGui::SetCursorScreenPos(origin);
    }
    if (displaySize.x > viewportMargin && origin.x + width > displaySize.x - viewportMargin)
        width = (std::max)(1.0F, displaySize.x - viewportMargin - origin.x);

    auto* draw = ImGui::GetForegroundDrawList();
    if (!draw) {
        ImGui::Dummy(ImVec2{width, height});
        return;
    }

    const ImVec2 max{origin.x + width, origin.y + height};
    draw->AddRectFilled({origin.x + 5.0F, origin.y + 6.0F}, {max.x + 5.0F, max.y + 6.0F},
        ImGui::GetColorU32(ImVec4{0.0F, 0.0F, 0.0F, 0.66F}), 2.0F);

    const float sourceAspect = image.Height() > 0
        ? static_cast<float>(image.Width()) / static_cast<float>(image.Height())
        : 1.0F;
    const float targetAspect = width / height;
    ImVec2 uv0{0.0F, 0.0F};
    ImVec2 uv1{1.0F, 1.0F};
    if (sourceAspect > targetAspect) {
        const float visible = targetAspect / sourceAspect;
        uv0.x = (1.0F - visible) * 0.5F;
        uv1.x = 1.0F - uv0.x;
    } else if (sourceAspect < targetAspect) {
        const float visible = sourceAspect / targetAspect;
        uv0.y = (1.0F - visible) * 0.5F;
        uv1.y = 1.0F - uv0.y;
    }

    const int alpha = static_cast<int>(255.0F * appearance.BannerOpacity());
    draw->AddImage(image.Texture(), origin, max, uv0, uv1, IM_COL32(255, 255, 255, alpha));
    draw->AddRectFilled(origin, max, ImGui::GetColorU32(ImVec4{0.015F, 0.008F, 0.006F, 0.26F}));
    draw->AddRectFilled({origin.x, origin.y + 79.0F}, max,
        ImGui::GetColorU32(ImVec4{0.01F, 0.006F, 0.004F, 0.62F}));
    theme.DrawOrnateFrame(draw, origin, max, true);

    const char* title = "DEVIL'S DEN MENU";
    const char* subtitle = "GTA V ENHANCED  |  RUNTIME READY";
    const auto titleSize = ImGui::CalcTextSize(title);
    const ImVec2 titlePos{origin.x + (width - titleSize.x) * 0.5F, origin.y + 82.0F};
    draw->AddText({titlePos.x + 2.0F, titlePos.y + 2.0F}, ImGui::GetColorU32(ImVec4{0, 0, 0, 0.95F}), title);
    draw->AddText(titlePos, ImGui::GetColorU32(palette.emberRed), title);

    const auto subtitleSize = ImGui::CalcTextSize(subtitle);
    draw->AddText({origin.x + (width - subtitleSize.x) * 0.5F, origin.y + 101.0F},
        ImGui::GetColorU32(palette.bronze), subtitle);

    ImGui::Dummy(ImVec2{width, height});
}
}
