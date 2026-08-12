#pragma once

#include "Frontend/Menu/Devils_Den_Config.hpp"
#include "Frontend/Menu/Themes/Menu_Theme.hpp"
#include "Frontend/Renderer/D3D12_Image_Loader.hpp"

#include <imgui.h>

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace Devilz::Backend
{
class LoggerService;
}

namespace Devilz::Frontend
{
enum class Appearance_Image_Category
{
    Banner,
    Background
};

struct Appearance_Image_Asset
{
    std::string name;
    std::filesystem::path path;
};

struct Appearance_Font_Asset
{
    std::string fileName;
    std::string displayName;
    std::filesystem::path path;
};

class Menu_Appearance_State final
{
public:
    static Menu_Appearance_State& Instance() noexcept;

    void ConfigureLogger(Backend::LoggerService* logger) noexcept;
    void InitializeOnce() noexcept;
    void Tick() noexcept;
    void Shutdown() noexcept;

    [[nodiscard]] static std::filesystem::path RootDirectory();
    [[nodiscard]] static std::filesystem::path ImagesDirectory();
    [[nodiscard]] static std::filesystem::path BannersDirectory();
    [[nodiscard]] static std::filesystem::path BackgroundsDirectory();
    [[nodiscard]] static std::filesystem::path IconsDirectory();
    [[nodiscard]] static std::filesystem::path WindowsFontsDirectory();

    void RefreshImages() noexcept;
    void RefreshFonts() noexcept;
    [[nodiscard]] bool ImportImage(
        Appearance_Image_Category category,
        const std::filesystem::path& source,
        std::string* importedName = nullptr,
        std::string* error = nullptr) noexcept;

    [[nodiscard]] const std::vector<Appearance_Image_Asset>& Banners() const noexcept { return m_banners; }
    [[nodiscard]] const std::vector<Appearance_Image_Asset>& Backgrounds() const noexcept { return m_backgrounds; }
    [[nodiscard]] const std::vector<std::string>& IconSets() const noexcept { return m_iconSets; }
    [[nodiscard]] const std::vector<Appearance_Font_Asset>& Fonts() const noexcept { return m_fonts; }

    [[nodiscard]] Renderer::D3D12_Image_View ImageView(const Appearance_Image_Asset& asset) noexcept;
    [[nodiscard]] Renderer::D3D12_Image_View BannerView() noexcept;
    [[nodiscard]] Renderer::D3D12_Image_View BackgroundView() noexcept;
    [[nodiscard]] std::filesystem::path IconSetPreviewPath(const std::string& set) const;

    void SetTheme(int theme) noexcept;
    [[nodiscard]] int Theme() const noexcept { return m_theme; }

    void SelectBanner(const std::string& fileName, bool persist = true) noexcept;
    void SetBannerEnabled(bool enabled) noexcept;
    void SetBannerOpacity(float opacity) noexcept;
    [[nodiscard]] bool BannerEnabled() const noexcept { return m_bannerEnabled && !m_bannerImage.empty(); }
    [[nodiscard]] float BannerOpacity() const noexcept { return m_bannerOpacity; }
    [[nodiscard]] const std::string& BannerImage() const noexcept { return m_bannerImage; }

    void SelectBackground(const std::string& fileName, bool persist = true) noexcept;
    void SetBackgroundEnabled(bool enabled) noexcept;
    void SetBackgroundOpacity(float opacity) noexcept;
    void SetBackgroundFit(int fit) noexcept;
    [[nodiscard]] bool BackgroundEnabled() const noexcept { return m_backgroundEnabled && !m_backgroundImage.empty(); }
    [[nodiscard]] float BackgroundOpacity() const noexcept { return m_backgroundOpacity; }
    [[nodiscard]] int BackgroundFit() const noexcept { return m_backgroundFit; }
    [[nodiscard]] const std::string& BackgroundImage() const noexcept { return m_backgroundImage; }
    void DrawBackground(ImDrawList* draw, ImVec2 min, ImVec2 max) noexcept;

    void SelectIconSet(const std::string& set, bool persist = true) noexcept;
    void SetIconsEnabled(bool enabled) noexcept;
    [[nodiscard]] bool IconsEnabled() const noexcept { return m_iconsEnabled && !m_iconSet.empty(); }
    [[nodiscard]] const std::string& IconSet() const noexcept { return m_iconSet; }
    [[nodiscard]] bool DrawCustomIcon(
        Themes::Menu_Icon icon,
        ImDrawList* draw,
        ImVec2 center,
        float radius,
        float alpha = 1.0F) noexcept;

    void SelectFont(const std::string& fileName) noexcept;
    void SetFontSize(float size) noexcept;
    [[nodiscard]] const std::string& SelectedFontFile() const noexcept { return m_fontFile; }
    [[nodiscard]] float FontSize() const noexcept { return m_fontSize; }
    [[nodiscard]] ImFont* PreviewFont(const Appearance_Font_Asset& asset) noexcept;
    [[nodiscard]] bool ApplySelectedFont(bool persist = true) noexcept;
    void ResetFont(bool persist = true) noexcept;

    [[nodiscard]] ImGuiStyle& StyleReference() noexcept { return m_styleReference; }
    void CaptureStyleFromImGui(bool persist = true) noexcept;
    void ApplyStoredStyle() noexcept;
    void ReloadStyleFromSettings() noexcept;
    void ResetStyleToTheme(bool persist = true) noexcept;

    void Save() noexcept;
    void ReloadAll() noexcept;
    void CopyToConfig(Devils_Den_Config& config) const;
    void ApplyFromConfig(const Devils_Den_Config& config) noexcept;

    [[nodiscard]] const std::string& Status() const noexcept { return m_status; }

private:
    Menu_Appearance_State() = default;

    [[nodiscard]] static bool IsImageExtension(const std::filesystem::path& path) noexcept;
    [[nodiscard]] static bool IsFontExtension(const std::filesystem::path& path) noexcept;
    [[nodiscard]] static std::string SerializeStyle(const ImGuiStyle& style);
    [[nodiscard]] static bool DeserializeStyle(const std::string& data, ImGuiStyle& style) noexcept;
    [[nodiscard]] std::filesystem::path ResolveBannerPath() const;
    [[nodiscard]] std::filesystem::path ResolveBackgroundPath() const;
    [[nodiscard]] std::filesystem::path ResolveFontPath() const;
    [[nodiscard]] std::filesystem::path ResolveIconPath(Themes::Menu_Icon icon) const;
    [[nodiscard]] bool HasBanner(const std::string& name) const noexcept;
    [[nodiscard]] bool HasBackground(const std::string& name) const noexcept;
    [[nodiscard]] bool HasIconSet(const std::string& name) const noexcept;
    [[nodiscard]] bool HasFont(const std::string& name) const noexcept;
    void EnsureDirectories() noexcept;
    void ApplyThemeAndStyle() noexcept;
    void LogInfo(const std::string& message, const char* service) noexcept;
    void LogWarning(const std::string& message, const char* service) noexcept;
    void LogError(const std::string& message, const char* service) noexcept;

    Backend::LoggerService* m_logger = nullptr;
    bool m_initialized = false;
    int m_theme = 0;

    bool m_bannerEnabled = false;
    std::string m_bannerImage;
    float m_bannerOpacity = 1.0F;

    bool m_backgroundEnabled = false;
    std::string m_backgroundImage;
    float m_backgroundOpacity = 0.30F;
    int m_backgroundFit = 0;

    bool m_iconsEnabled = false;
    std::string m_iconSet;

    std::string m_fontFile;
    float m_fontSize = 18.0F;
    ImFont* m_activeFont = nullptr;
    std::unordered_map<std::string, ImFont*> m_previewFonts;

    ImGuiStyle m_savedStyle{};
    ImGuiStyle m_styleReference{};
    std::string m_styleData;

    std::vector<Appearance_Image_Asset> m_banners;
    std::vector<Appearance_Image_Asset> m_backgrounds;
    std::vector<std::string> m_iconSets;
    std::vector<Appearance_Font_Asset> m_fonts;
    std::unordered_map<std::string, std::filesystem::path> m_iconPaths;

    std::string m_status = "Appearance manager ready.";
};
}
