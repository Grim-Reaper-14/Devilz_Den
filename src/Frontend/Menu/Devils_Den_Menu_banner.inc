#include "Frontend/Menu/Menu_Appearance.hpp"

namespace Devilz::Frontend
{
void Devils_Den_Menu::DrawBanner()
{
    auto& appearance = Menu_Appearance_State::Instance();
    appearance.InitializeOnce();

    if (!appearance.BannerEnabled()) {
        DrawBannerLegacy();
        return;
    }

    const auto view = appearance.BannerView();
    if (!view.ready) {
        DrawBannerLegacy();
        return;
    }

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

    auto* draw = ImGui::GetWindowDrawList();
    if (!draw) {
        ImGui::Dummy(ImVec2{width, height});
        return;
    }

    ImVec2 uv0{0.0F, 0.0F};
    ImVec2 uv1{1.0F, 1.0F};
    const float sourceAspect = view.height > 0
        ? static_cast<float>(view.width) / static_cast<float>(view.height) : 1.0F;
    const float targetAspect = width / height;
    if (sourceAspect > targetAspect) {
        const float visible = targetAspect / sourceAspect;
        uv0.x = (1.0F - visible) * 0.5F;
        uv1.x = 1.0F - uv0.x;
    } else if (sourceAspect < targetAspect) {
        const float visible = sourceAspect / targetAspect;
        uv0.y = (1.0F - visible) * 0.5F;
        uv1.y = 1.0F - uv0.y;
    }

    const ImVec2 max{origin.x + width, origin.y + height};
    const int alpha = static_cast<int>(255.0F * appearance.BannerOpacity());
    draw->AddImage(view.texture, origin, max, uv0, uv1, IM_COL32(255, 255, 255, alpha));
    ImGui::Dummy(ImVec2{width, height});
}
}
