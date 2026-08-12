#pragma once

#include "Frontend/Menu/Devils_Den_Config.hpp"
#include "Frontend/Menu/Themes/Menu_Theme.hpp"
#include "Frontend/Renderer/D3D12_Image_Loader.hpp"

#include <algorithm>
#include <string>

namespace Devilz::Frontend
{
class Menu_Appearance_State final
{
public:
    static Menu_Appearance_State& Instance() noexcept
    {
        static Menu_Appearance_State state;
        return state;
    }

    void InitializeOnce() noexcept
    {
        if (m_initialized)
            return;
        m_initialized = true;

        Devils_Den_Config config{};
        if (Devils_Den_Config_Store::LoadAppearance(config)) {
            m_theme = std::clamp(config.menuTheme, 0, Themes::Menu_Theme_Count - 1);
            m_bannerEnabled = config.bannerEnabled;
            m_bannerPath = config.bannerImagePath;
            m_bannerOpacity = std::clamp(config.bannerOpacity, 0.10F, 1.0F);
        }
        Apply(false);
    }

    void Apply(bool persist = true) noexcept
    {
        Themes::Menu_Theme_Manager::Instance().ApplyPreset(static_cast<Themes::Menu_Theme_Id>(m_theme));

        if (m_bannerEnabled && !m_bannerPath.empty()) {
            std::string error;
            m_bannerLoaded = Renderer::D3D12_Image_Loader::Instance().Load(m_bannerPath, &error);
            m_status = m_bannerLoaded ? "Banner image loaded." : error;
        } else {
            Renderer::D3D12_Image_Loader::Instance().Clear();
            m_bannerLoaded = false;
            if (!m_bannerEnabled)
                m_status = "Procedural banner active.";
        }

        if (persist)
            Save();
    }

    void Save() noexcept
    {
        Devils_Den_Config config{};
        config.menuTheme = m_theme;
        config.bannerEnabled = m_bannerEnabled;
        config.bannerImagePath = m_bannerPath;
        config.bannerOpacity = m_bannerOpacity;
        std::string error;
        if (!Devils_Den_Config_Store::SaveAppearance(config, &error) && !error.empty())
            m_status = error;
    }

    void SetTheme(int theme) noexcept
    {
        m_theme = std::clamp(theme, 0, Themes::Menu_Theme_Count - 1);
        Themes::Menu_Theme_Manager::Instance().ApplyPreset(static_cast<Themes::Menu_Theme_Id>(m_theme));
        Save();
    }

    bool LoadBanner(const std::string& path) noexcept
    {
        m_bannerPath = path;
        m_bannerEnabled = !path.empty();
        std::string error;
        m_bannerLoaded = m_bannerEnabled && Renderer::D3D12_Image_Loader::Instance().Load(path, &error);
        m_status = m_bannerLoaded ? "Banner image loaded." : (error.empty() ? "Banner image disabled." : error);
        Save();
        return m_bannerLoaded;
    }

    void ClearBanner() noexcept
    {
        m_bannerEnabled = false;
        m_bannerPath.clear();
        m_bannerLoaded = false;
        Renderer::D3D12_Image_Loader::Instance().Clear();
        m_status = "Procedural banner active.";
        Save();
    }

    void SetBannerEnabled(bool enabled) noexcept
    {
        m_bannerEnabled = enabled;
        Apply(true);
    }

    void SetBannerOpacity(float opacity) noexcept
    {
        m_bannerOpacity = std::clamp(opacity, 0.10F, 1.0F);
        Save();
    }

    [[nodiscard]] int Theme() const noexcept { return m_theme; }
    [[nodiscard]] bool BannerEnabled() const noexcept { return m_bannerEnabled; }
    [[nodiscard]] bool BannerLoaded() const noexcept { return m_bannerLoaded && Renderer::D3D12_Image_Loader::Instance().Ready(); }
    [[nodiscard]] float BannerOpacity() const noexcept { return m_bannerOpacity; }
    [[nodiscard]] const std::string& BannerPath() const noexcept { return m_bannerPath; }
    [[nodiscard]] const std::string& Status() const noexcept { return m_status; }

    void CopyToConfig(Devils_Den_Config& config) const
    {
        config.menuTheme = m_theme;
        config.bannerEnabled = m_bannerEnabled;
        config.bannerImagePath = m_bannerPath;
        config.bannerOpacity = m_bannerOpacity;
    }

    void ApplyFromConfig(const Devils_Den_Config& config) noexcept
    {
        m_theme = std::clamp(config.menuTheme, 0, Themes::Menu_Theme_Count - 1);
        m_bannerEnabled = config.bannerEnabled;
        m_bannerPath = config.bannerImagePath;
        m_bannerOpacity = std::clamp(config.bannerOpacity, 0.10F, 1.0F);
        Apply(true);
    }

private:
    bool m_initialized = false;
    int m_theme = 0;
    bool m_bannerEnabled = false;
    bool m_bannerLoaded = false;
    float m_bannerOpacity = 1.0F;
    std::string m_bannerPath;
    std::string m_status = "Procedural banner active.";
};
}
