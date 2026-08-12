#include "Menu_Appearance.hpp"

#include "Backend/Logging/LoggerService.hpp"

#include <Windows.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <system_error>
#include <type_traits>

namespace Devilz::Frontend
{
namespace
{
std::string Lower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

const char* IconStem(Themes::Menu_Icon icon) noexcept
{
    switch (icon) {
    case Themes::Menu_Icon::DevilCrest: return "devilcrest";
    case Themes::Menu_Icon::Self: return "self";
    case Themes::Menu_Icon::Weapons: return "weapons";
    case Themes::Menu_Icon::Vehicle: return "vehicle";
    case Themes::Menu_Icon::Teleport: return "teleport";
    case Themes::Menu_Icon::World: return "world";
    case Themes::Menu_Icon::Network: return "network";
    case Themes::Menu_Icon::Lua: return "lua";
    case Themes::Menu_Icon::Settings: return "settings";
    default: return "icon";
    }
}

void CropUvs(int sourceWidth, int sourceHeight, float targetAspect, ImVec2& uv0, ImVec2& uv1) noexcept
{
    if (sourceWidth <= 0 || sourceHeight <= 0 || targetAspect <= 0.0F)
        return;
    const float sourceAspect = static_cast<float>(sourceWidth) / static_cast<float>(sourceHeight);
    if (sourceAspect > targetAspect) {
        const float visible = targetAspect / sourceAspect;
        uv0.x = (1.0F - visible) * 0.5F;
        uv1.x = 1.0F - uv0.x;
    } else if (sourceAspect < targetAspect) {
        const float visible = sourceAspect / targetAspect;
        uv0.y = (1.0F - visible) * 0.5F;
        uv1.y = 1.0F - uv0.y;
    }
}
}

Menu_Appearance_State& Menu_Appearance_State::Instance() noexcept
{
    static Menu_Appearance_State state;
    return state;
}

void Menu_Appearance_State::ConfigureLogger(Backend::LoggerService* logger) noexcept
{
    m_logger = logger;
}

std::filesystem::path Menu_Appearance_State::RootDirectory()
{
    return Devils_Den_Config_Store::RootDirectory().parent_path();
}

std::filesystem::path Menu_Appearance_State::ImagesDirectory()
{
    return RootDirectory() / "Images";
}

std::filesystem::path Menu_Appearance_State::BannersDirectory()
{
    return ImagesDirectory() / "Banners";
}

std::filesystem::path Menu_Appearance_State::BackgroundsDirectory()
{
    return ImagesDirectory() / "Backgrounds";
}

std::filesystem::path Menu_Appearance_State::IconsDirectory()
{
    return ImagesDirectory() / "Icons";
}

std::filesystem::path Menu_Appearance_State::WindowsFontsDirectory()
{
    wchar_t buffer[MAX_PATH]{};
    const UINT length = ::GetWindowsDirectoryW(buffer, MAX_PATH);
    if (length == 0 || length >= MAX_PATH)
        return L"C:\\Windows\\Fonts";
    return std::filesystem::path(buffer) / L"Fonts";
}

bool Menu_Appearance_State::IsImageExtension(const std::filesystem::path& path) noexcept
{
    const auto ext = Lower(path.extension().string());
    return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" ||
        ext == ".gif" || ext == ".tif" || ext == ".tiff";
}

bool Menu_Appearance_State::IsFontExtension(const std::filesystem::path& path) noexcept
{
    const auto ext = Lower(path.extension().string());
    return ext == ".ttf" || ext == ".otf" || ext == ".ttc";
}

void Menu_Appearance_State::EnsureDirectories() noexcept
{
    std::error_code ec;
    std::filesystem::create_directories(BannersDirectory(), ec);
    if (ec) LogError("Could not create Images\\Banners: " + ec.message(), "Appearance.Images");
    ec.clear();
    std::filesystem::create_directories(BackgroundsDirectory(), ec);
    if (ec) LogError("Could not create Images\\Backgrounds: " + ec.message(), "Appearance.Images");
    ec.clear();
    std::filesystem::create_directories(IconsDirectory() / "Default", ec);
    if (ec) LogError("Could not create Images\\Icons\\Default: " + ec.message(), "Appearance.Images");
}

void Menu_Appearance_State::RefreshImages() noexcept
{
    EnsureDirectories();
    m_banners.clear();
    m_backgrounds.clear();
    m_iconSets.clear();
    m_iconPaths.clear();

    const auto scanImages = [](const std::filesystem::path& directory, std::vector<Appearance_Image_Asset>& out) {
        std::error_code ec;
        for (const auto& entry : std::filesystem::directory_iterator(directory, ec)) {
            if (ec) break;
            if (!entry.is_regular_file() || !Menu_Appearance_State::IsImageExtension(entry.path()))
                continue;
            out.push_back({entry.path().filename().string(), entry.path()});
        }
        std::sort(out.begin(), out.end(), [](const auto& lhs, const auto& rhs) {
            return Lower(lhs.name) < Lower(rhs.name);
        });
    };

    scanImages(BannersDirectory(), m_banners);
    scanImages(BackgroundsDirectory(), m_backgrounds);

    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(IconsDirectory(), ec)) {
        if (ec) break;
        if (!entry.is_directory())
            continue;
        const std::string set = entry.path().filename().string();
        m_iconSets.push_back(set);
        std::error_code iconEc;
        for (const auto& icon : std::filesystem::directory_iterator(entry.path(), iconEc)) {
            if (iconEc) break;
            if (!icon.is_regular_file() || !IsImageExtension(icon.path()))
                continue;
            const std::string key = Lower(set) + "/" + Lower(icon.path().stem().string());
            m_iconPaths[key] = icon.path();
        }
    }
    std::sort(m_iconSets.begin(), m_iconSets.end(), [](const auto& lhs, const auto& rhs) {
        return Lower(lhs) < Lower(rhs);
    });

    LogInfo("Image folders scanned | Banners: " + std::to_string(m_banners.size()) +
        " | Backgrounds: " + std::to_string(m_backgrounds.size()) +
        " | Icon sets: " + std::to_string(m_iconSets.size()), "Appearance.Images");
}

void Menu_Appearance_State::RefreshFonts() noexcept
{
    m_fonts.clear();
    const auto root = WindowsFontsDirectory();
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(root, ec)) {
        if (ec) break;
        if (!entry.is_regular_file() || !IsFontExtension(entry.path()))
            continue;
        Appearance_Font_Asset asset;
        asset.fileName = entry.path().filename().string();
        asset.displayName = entry.path().stem().string();
        asset.path = entry.path();
        m_fonts.push_back(std::move(asset));
    }
    std::sort(m_fonts.begin(), m_fonts.end(), [](const auto& lhs, const auto& rhs) {
        return Lower(lhs.displayName) < Lower(rhs.displayName);
    });
    LogInfo("Windows Fonts scanned | Usable files: " + std::to_string(m_fonts.size()) +
        " | Folder: " + root.string(), "Appearance.Fonts");
}

bool Menu_Appearance_State::ImportImage(
    Appearance_Image_Category category,
    const std::filesystem::path& source,
    std::string* importedName,
    std::string* error) noexcept
{
    if (!IsImageExtension(source)) {
        if (error) *error = "Unsupported image extension.";
        LogWarning("Import rejected: unsupported image extension: " + source.string(), "Appearance.Images");
        return false;
    }
    std::error_code ec;
    if (!std::filesystem::is_regular_file(source, ec) || ec) {
        if (error) *error = "Selected image does not exist.";
        LogWarning("Import failed: source image does not exist: " + source.string(), "Appearance.Images");
        return false;
    }

    const auto destinationRoot = category == Appearance_Image_Category::Banner
        ? BannersDirectory() : BackgroundsDirectory();
    std::filesystem::create_directories(destinationRoot, ec);
    if (ec) {
        if (error) *error = "Could not create managed image folder.";
        LogError("Import failed creating managed image folder: " + ec.message(), "Appearance.Images");
        return false;
    }

    const auto destination = destinationRoot / source.filename();
    const auto sourceKey = std::filesystem::weakly_canonical(source, ec);
    ec.clear();
    const auto destinationKey = std::filesystem::weakly_canonical(destination, ec);
    if (ec || sourceKey != destinationKey) {
        ec.clear();
        std::filesystem::copy_file(source, destination, std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) {
            if (error) *error = "Could not copy image into managed Images folder.";
            LogError("Image import copy failed: " + ec.message(), "Appearance.Images");
            return false;
        }
    }

    if (importedName)
        *importedName = destination.filename().string();
    RefreshImages();
    LogInfo("Imported image | Source: " + source.string() + " | Managed: " + destination.string(),
        "Appearance.Images");
    return true;
}

bool Menu_Appearance_State::HasBanner(const std::string& name) const noexcept
{
    return std::any_of(m_banners.begin(), m_banners.end(), [&](const auto& asset) { return asset.name == name; });
}

bool Menu_Appearance_State::HasBackground(const std::string& name) const noexcept
{
    return std::any_of(m_backgrounds.begin(), m_backgrounds.end(), [&](const auto& asset) { return asset.name == name; });
}

bool Menu_Appearance_State::HasIconSet(const std::string& name) const noexcept
{
    return std::find(m_iconSets.begin(), m_iconSets.end(), name) != m_iconSets.end();
}

bool Menu_Appearance_State::HasFont(const std::string& name) const noexcept
{
    return std::any_of(m_fonts.begin(), m_fonts.end(), [&](const auto& asset) { return asset.fileName == name; });
}

std::filesystem::path Menu_Appearance_State::ResolveBannerPath() const
{
    return m_bannerImage.empty() ? std::filesystem::path{} : BannersDirectory() / m_bannerImage;
}

std::filesystem::path Menu_Appearance_State::ResolveBackgroundPath() const
{
    return m_backgroundImage.empty() ? std::filesystem::path{} : BackgroundsDirectory() / m_backgroundImage;
}

std::filesystem::path Menu_Appearance_State::ResolveFontPath() const
{
    return m_fontFile.empty() ? std::filesystem::path{} : WindowsFontsDirectory() / m_fontFile;
}

std::filesystem::path Menu_Appearance_State::ResolveIconPath(Themes::Menu_Icon icon) const
{
    if (!IconsEnabled())
        return {};
    const std::string key = Lower(m_iconSet) + "/" + IconStem(icon);
    const auto found = m_iconPaths.find(key);
    return found == m_iconPaths.end() ? std::filesystem::path{} : found->second;
}

Renderer::D3D12_Image_View Menu_Appearance_State::ImageView(const Appearance_Image_Asset& asset) noexcept
{
    return Renderer::D3D12_Image_Loader::Instance().View(asset.path);
}

Renderer::D3D12_Image_View Menu_Appearance_State::BannerView() noexcept
{
    if (!BannerEnabled())
        return {};
    return Renderer::D3D12_Image_Loader::Instance().View(ResolveBannerPath());
}

Renderer::D3D12_Image_View Menu_Appearance_State::BackgroundView() noexcept
{
    if (!BackgroundEnabled())
        return {};
    return Renderer::D3D12_Image_Loader::Instance().View(ResolveBackgroundPath());
}

std::filesystem::path Menu_Appearance_State::IconSetPreviewPath(const std::string& set) const
{
    static constexpr const char* preferred[] = {"settings", "self", "devilcrest", "weapons"};
    for (const char* stem : preferred) {
        const auto found = m_iconPaths.find(Lower(set) + "/" + stem);
        if (found != m_iconPaths.end())
            return found->second;
    }
    const std::string prefix = Lower(set) + "/";
    for (const auto& [key, path] : m_iconPaths)
        if (key.rfind(prefix, 0) == 0)
            return path;
    return {};
}

std::string Menu_Appearance_State::SerializeStyle(const ImGuiStyle& style)
{
    static_assert(std::is_trivially_copyable_v<ImGuiStyle>);
    static constexpr char hex[] = "0123456789ABCDEF";
    const auto* bytes = reinterpret_cast<const unsigned char*>(&style);
    std::string out = std::to_string(IMGUI_VERSION_NUM) + ":" + std::to_string(sizeof(ImGuiStyle)) + ":";
    out.reserve(out.size() + sizeof(ImGuiStyle) * 2U);
    for (std::size_t index = 0; index < sizeof(ImGuiStyle); ++index) {
        out.push_back(hex[(bytes[index] >> 4U) & 0x0FU]);
        out.push_back(hex[bytes[index] & 0x0FU]);
    }
    return out;
}

bool Menu_Appearance_State::DeserializeStyle(const std::string& data, ImGuiStyle& style) noexcept
{
    static_assert(std::is_trivially_copyable_v<ImGuiStyle>);
    try {
        const auto first = data.find(':');
        const auto second = first == std::string::npos ? std::string::npos : data.find(':', first + 1U);
        if (first == std::string::npos || second == std::string::npos)
            return false;
        if (std::stoi(data.substr(0, first)) != IMGUI_VERSION_NUM)
            return false;
        if (std::stoull(data.substr(first + 1U, second - first - 1U)) != sizeof(ImGuiStyle))
            return false;
        const auto hexData = data.substr(second + 1U);
        if (hexData.size() != sizeof(ImGuiStyle) * 2U)
            return false;

        auto nibble = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            return -1;
        };
        unsigned char bytes[sizeof(ImGuiStyle)]{};
        for (std::size_t index = 0; index < sizeof(ImGuiStyle); ++index) {
            const int hi = nibble(hexData[index * 2U]);
            const int lo = nibble(hexData[index * 2U + 1U]);
            if (hi < 0 || lo < 0)
                return false;
            bytes[index] = static_cast<unsigned char>((hi << 4) | lo);
        }
        ImGuiStyle decoded;
        std::memcpy(&decoded, bytes, sizeof(decoded));
        style = decoded;
        return true;
    } catch (...) {
        return false;
    }
}

void Menu_Appearance_State::ApplyThemeAndStyle() noexcept
{
    auto& theme = Themes::Menu_Theme_Manager::Instance();
    theme.ApplyPreset(static_cast<Themes::Menu_Theme_Id>(std::clamp(m_theme, 0, Themes::Menu_Theme_Count - 1)));
    theme.Apply();
    m_styleReference = ImGui::GetStyle();

    ImGuiStyle restored;
    if (!m_styleData.empty() && DeserializeStyle(m_styleData, restored)) {
        m_savedStyle = restored;
        ImGui::GetStyle() = m_savedStyle;
        LogInfo("Restored saved ImGui style for Dear ImGui " IMGUI_VERSION, "Appearance.Style");
    } else {
        m_savedStyle = ImGui::GetStyle();
        m_styleData = SerializeStyle(m_savedStyle);
    }
}

void Menu_Appearance_State::InitializeOnce() noexcept
{
    if (m_initialized || !ImGui::GetCurrentContext())
        return;
    m_initialized = true;

    EnsureDirectories();
    RefreshImages();
    RefreshFonts();

    Devils_Den_Config config{};
    std::string error;
    if (Devils_Den_Config_Store::LoadAppearance(config, &error)) {
        m_theme = std::clamp(config.menuTheme, 0, Themes::Menu_Theme_Count - 1);
        m_bannerEnabled = config.bannerEnabled;
        m_bannerImage = config.bannerImage;
        m_bannerOpacity = std::clamp(config.bannerOpacity, 0.10F, 1.0F);
        m_backgroundEnabled = config.backgroundEnabled;
        m_backgroundImage = config.backgroundImage;
        m_backgroundOpacity = std::clamp(config.backgroundOpacity, 0.0F, 1.0F);
        m_backgroundFit = std::clamp(config.backgroundFit, 0, 2);
        m_iconsEnabled = config.iconsEnabled;
        m_iconSet = config.iconSet;
        m_fontFile = config.fontFile;
        m_fontSize = std::clamp(config.fontSize, 10.0F, 42.0F);
        m_styleData = config.imguiStyleData;

        if (m_bannerImage.empty() && !config.bannerImagePath.empty()) {
            std::string imported;
            std::string migrationError;
            if (ImportImage(Appearance_Image_Category::Banner, config.bannerImagePath, &imported, &migrationError)) {
                m_bannerImage = imported;
                LogInfo("Migrated legacy banner path into managed Images\\Banners: " + imported,
                    "Appearance.Images");
            } else {
                LogWarning("Legacy banner migration failed: " + migrationError, "Appearance.Images");
            }
        }
        LogInfo("Loaded previous appearance session from " + Devils_Den_Config_Store::SettingsPath().string(),
            "Appearance.Config");
    } else {
        LogInfo("No previous appearance session found; using defaults.", "Appearance.Config");
    }

    if (!m_bannerImage.empty() && !HasBanner(m_bannerImage)) {
        LogWarning("Saved banner is missing from Images\\Banners: " + m_bannerImage, "Appearance.Images");
        m_bannerEnabled = false;
    }
    if (!m_backgroundImage.empty() && !HasBackground(m_backgroundImage)) {
        LogWarning("Saved background is missing from Images\\Backgrounds: " + m_backgroundImage,
            "Appearance.Images");
        m_backgroundEnabled = false;
    }
    if (!m_iconSet.empty() && !HasIconSet(m_iconSet)) {
        LogWarning("Saved icon set is missing from Images\\Icons: " + m_iconSet, "Appearance.Images");
        m_iconsEnabled = false;
    }

    ApplyThemeAndStyle();
    if (!m_fontFile.empty() && HasFont(m_fontFile))
        (void)ApplySelectedFont(false);
    else if (!m_fontFile.empty()) {
        LogWarning("Saved font is missing; using ImGui default: " + m_fontFile, "Appearance.Fonts");
        ResetFont(false);
    }

    Save();
    m_status = "Restored last saved appearance session.";
}

void Menu_Appearance_State::Tick() noexcept
{
    Renderer::D3D12_Image_Loader::Instance().Tick();
}

void Menu_Appearance_State::Shutdown() noexcept
{
    m_previewFonts.clear();
    m_activeFont = nullptr;
    m_initialized = false;
}

void Menu_Appearance_State::SetTheme(int theme) noexcept
{
    m_theme = std::clamp(theme, 0, Themes::Menu_Theme_Count - 1);
    auto& manager = Themes::Menu_Theme_Manager::Instance();
    manager.ApplyPreset(static_cast<Themes::Menu_Theme_Id>(m_theme));
    manager.Apply();
    m_savedStyle = ImGui::GetStyle();
    m_styleReference = m_savedStyle;
    m_styleData = SerializeStyle(m_savedStyle);
    LogInfo("Theme applied: " + std::string(Themes::MenuThemeName(static_cast<Themes::Menu_Theme_Id>(m_theme))),
        "Appearance.Theme");
    Save();
}

void Menu_Appearance_State::SelectBanner(const std::string& fileName, bool persist) noexcept
{
    if (!fileName.empty() && !HasBanner(fileName)) {
        m_status = "Banner is not present in Images\\Banners.";
        LogWarning("Banner selection rejected: " + fileName, "Appearance.Images");
        return;
    }
    m_bannerImage = fileName;
    m_bannerEnabled = !fileName.empty();
    if (m_bannerEnabled) {
        const auto view = BannerView();
        if (view.ready) {
            m_status = "Banner applied: " + fileName;
            LogInfo("Banner selected and texture queued: " + fileName + " | " +
                std::to_string(view.width) + "x" + std::to_string(view.height), "Appearance.Images");
        } else {
            m_status = "Banner selected but texture could not be loaded.";
            LogError("Banner texture load failed: " + fileName, "Appearance.Images");
        }
    } else {
        m_status = "Procedural banner active.";
        LogInfo("Custom banner disabled.", "Appearance.Images");
    }
    if (persist) Save();
}

void Menu_Appearance_State::SetBannerEnabled(bool enabled) noexcept
{
    m_bannerEnabled = enabled && !m_bannerImage.empty();
    LogInfo(std::string("Banner image ") + (m_bannerEnabled ? "enabled" : "disabled"), "Appearance.Images");
    Save();
}

void Menu_Appearance_State::SetBannerOpacity(float opacity) noexcept
{
    m_bannerOpacity = std::clamp(opacity, 0.10F, 1.0F);
    Save();
}

void Menu_Appearance_State::SelectBackground(const std::string& fileName, bool persist) noexcept
{
    if (!fileName.empty() && !HasBackground(fileName)) {
        m_status = "Background is not present in Images\\Backgrounds.";
        LogWarning("Background selection rejected: " + fileName, "Appearance.Images");
        return;
    }
    m_backgroundImage = fileName;
    m_backgroundEnabled = !fileName.empty();
    if (m_backgroundEnabled) {
        const auto view = BackgroundView();
        m_status = view.ready ? "Background applied: " + fileName : "Background texture could not be loaded.";
        LogInfo("Background selected: " + fileName, "Appearance.Images");
    } else {
        m_status = "Menu background image disabled.";
        LogInfo("Menu background image disabled.", "Appearance.Images");
    }
    if (persist) Save();
}

void Menu_Appearance_State::SetBackgroundEnabled(bool enabled) noexcept
{
    m_backgroundEnabled = enabled && !m_backgroundImage.empty();
    LogInfo(std::string("Background image ") + (m_backgroundEnabled ? "enabled" : "disabled"),
        "Appearance.Images");
    Save();
}

void Menu_Appearance_State::SetBackgroundOpacity(float opacity) noexcept
{
    m_backgroundOpacity = std::clamp(opacity, 0.0F, 1.0F);
    Save();
}

void Menu_Appearance_State::SetBackgroundFit(int fit) noexcept
{
    m_backgroundFit = std::clamp(fit, 0, 2);
    Save();
}

void Menu_Appearance_State::DrawBackground(ImDrawList* draw, ImVec2 min, ImVec2 max) noexcept
{
    if (!draw || !BackgroundEnabled())
        return;
    const auto view = BackgroundView();
    if (!view.ready)
        return;

    ImVec2 targetMin = min;
    ImVec2 targetMax = max;
    ImVec2 uv0{0.0F, 0.0F};
    ImVec2 uv1{1.0F, 1.0F};
    const float targetWidth = (std::max)(1.0F, max.x - min.x);
    const float targetHeight = (std::max)(1.0F, max.y - min.y);
    const float targetAspect = targetWidth / targetHeight;

    if (m_backgroundFit == 0) {
        CropUvs(view.width, view.height, targetAspect, uv0, uv1);
    } else if (m_backgroundFit == 1 && view.width > 0 && view.height > 0) {
        const float sourceAspect = static_cast<float>(view.width) / static_cast<float>(view.height);
        if (sourceAspect > targetAspect) {
            const float h = targetWidth / sourceAspect;
            targetMin.y += (targetHeight - h) * 0.5F;
            targetMax.y = targetMin.y + h;
        } else {
            const float w = targetHeight * sourceAspect;
            targetMin.x += (targetWidth - w) * 0.5F;
            targetMax.x = targetMin.x + w;
        }
    }

    const int alpha = static_cast<int>(255.0F * std::clamp(m_backgroundOpacity, 0.0F, 1.0F));
    draw->AddImage(view.texture, targetMin, targetMax, uv0, uv1, IM_COL32(255, 255, 255, alpha));
}

void Menu_Appearance_State::SelectIconSet(const std::string& set, bool persist) noexcept
{
    if (!set.empty() && !HasIconSet(set)) {
        m_status = "Icon set is not present in Images\\Icons.";
        LogWarning("Icon set selection rejected: " + set, "Appearance.Images");
        return;
    }
    m_iconSet = set;
    m_iconsEnabled = !set.empty();
    m_status = m_iconsEnabled ? "Icon set applied: " + set : "Procedural icons active.";
    LogInfo(m_status, "Appearance.Images");
    if (persist) Save();
}

void Menu_Appearance_State::SetIconsEnabled(bool enabled) noexcept
{
    m_iconsEnabled = enabled && !m_iconSet.empty();
    Save();
}

bool Menu_Appearance_State::DrawCustomIcon(
    Themes::Menu_Icon icon,
    ImDrawList* draw,
    ImVec2 center,
    float radius,
    float alpha) noexcept
{
    if (!draw || radius <= 0.0F)
        return false;
    const auto path = ResolveIconPath(icon);
    if (path.empty())
        return false;
    const auto view = Renderer::D3D12_Image_Loader::Instance().View(path);
    if (!view.ready)
        return false;
    ImVec2 uv0{0.0F, 0.0F};
    ImVec2 uv1{1.0F, 1.0F};
    CropUvs(view.width, view.height, 1.0F, uv0, uv1);
    const int a = static_cast<int>(255.0F * std::clamp(alpha, 0.0F, 1.0F));
    draw->AddImage(view.texture,
        {center.x - radius, center.y - radius},
        {center.x + radius, center.y + radius}, uv0, uv1, IM_COL32(255, 255, 255, a));
    return true;
}

void Menu_Appearance_State::SelectFont(const std::string& fileName) noexcept
{
    if (!fileName.empty() && !HasFont(fileName)) {
        m_status = "Font is not present in the Windows Fonts folder.";
        return;
    }
    m_fontFile = fileName;
    m_status = fileName.empty() ? "Default ImGui font selected." : "Font selected for preview: " + fileName;
}

void Menu_Appearance_State::SetFontSize(float size) noexcept
{
    m_fontSize = std::clamp(size, 10.0F, 42.0F);
}

ImFont* Menu_Appearance_State::PreviewFont(const Appearance_Font_Asset& asset) noexcept
{
    if (asset.fileName == m_fontFile && m_activeFont)
        return m_activeFont;
    const auto found = m_previewFonts.find(asset.fileName);
    if (found != m_previewFonts.end())
        return found->second;
    if (!ImGui::GetCurrentContext() || m_previewFonts.size() >= 32U)
        return nullptr;
    auto& io = ImGui::GetIO();
    const auto path = asset.path.string();
    ImFont* font = io.Fonts->AddFontFromFileTTF(path.c_str(), 18.0F);
    if (font)
        m_previewFonts.emplace(asset.fileName, font);
    return font;
}

bool Menu_Appearance_State::ApplySelectedFont(bool persist) noexcept
{
    if (!ImGui::GetCurrentContext())
        return false;
    if (m_fontFile.empty()) {
        ResetFont(persist);
        return true;
    }
    const auto path = ResolveFontPath();
    std::error_code ec;
    if (!std::filesystem::is_regular_file(path, ec) || ec) {
        m_status = "Selected Windows font no longer exists.";
        LogWarning("Font apply failed; file missing: " + path.string(), "Appearance.Fonts");
        return false;
    }
    auto& io = ImGui::GetIO();
    const auto pathText = path.string();
    ImFont* font = io.Fonts->AddFontFromFileTTF(pathText.c_str(), m_fontSize);
    if (!font) {
        m_status = "ImGui could not load selected font.";
        LogError("ImGui font load failed: " + pathText, "Appearance.Fonts");
        return false;
    }
    io.FontDefault = font;
    m_activeFont = font;
    m_status = "Font applied: " + m_fontFile;
    LogInfo("Font applied | " + m_fontFile + " | Size: " + std::to_string(m_fontSize), "Appearance.Fonts");
    if (persist) Save();
    return true;
}

void Menu_Appearance_State::ResetFont(bool persist) noexcept
{
    if (ImGui::GetCurrentContext())
        ImGui::GetIO().FontDefault = nullptr;
    m_fontFile.clear();
    m_activeFont = nullptr;
    m_status = "Default ImGui font active.";
    LogInfo("Font reset to ImGui default.", "Appearance.Fonts");
    if (persist) Save();
}

void Menu_Appearance_State::CaptureStyleFromImGui(bool persist) noexcept
{
    if (!ImGui::GetCurrentContext())
        return;
    m_savedStyle = ImGui::GetStyle();
    m_styleReference = m_savedStyle;
    m_styleData = SerializeStyle(m_savedStyle);
    m_status = "Current ImGui style captured and applied.";
    LogInfo("Captured current ImGui style.", "Appearance.Style");
    if (persist) Save();
}

void Menu_Appearance_State::ApplyStoredStyle() noexcept
{
    if (!ImGui::GetCurrentContext())
        return;
    ImGui::GetStyle() = m_savedStyle;
    m_status = "Saved ImGui style applied.";
    LogInfo("Applied saved ImGui style.", "Appearance.Style");
}

void Menu_Appearance_State::ReloadStyleFromSettings() noexcept
{
    Devils_Den_Config config{};
    std::string error;
    if (!Devils_Den_Config_Store::LoadAppearance(config, &error)) {
        m_status = error;
        LogWarning("Style reload failed: " + error, "Appearance.Style");
        return;
    }
    ImGuiStyle restored;
    if (!DeserializeStyle(config.imguiStyleData, restored)) {
        m_status = "Saved style data is missing or incompatible.";
        LogWarning(m_status, "Appearance.Style");
        return;
    }
    m_savedStyle = restored;
    m_styleReference = restored;
    m_styleData = config.imguiStyleData;
    ImGui::GetStyle() = restored;
    m_status = "ImGui style reloaded from Settings.json.";
    LogInfo(m_status, "Appearance.Style");
}

void Menu_Appearance_State::ResetStyleToTheme(bool persist) noexcept
{
    auto& manager = Themes::Menu_Theme_Manager::Instance();
    manager.ApplyPreset(static_cast<Themes::Menu_Theme_Id>(m_theme));
    manager.Apply();
    m_savedStyle = ImGui::GetStyle();
    m_styleReference = m_savedStyle;
    m_styleData = SerializeStyle(m_savedStyle);
    m_status = "ImGui style reset to current theme.";
    LogInfo(m_status, "Appearance.Style");
    if (persist) Save();
}

void Menu_Appearance_State::CopyToConfig(Devils_Den_Config& config) const
{
    config.version = 3;
    config.menuTheme = m_theme;
    config.bannerEnabled = m_bannerEnabled;
    config.bannerImage = m_bannerImage;
    config.bannerOpacity = m_bannerOpacity;
    config.backgroundEnabled = m_backgroundEnabled;
    config.backgroundImage = m_backgroundImage;
    config.backgroundOpacity = m_backgroundOpacity;
    config.backgroundFit = m_backgroundFit;
    config.iconsEnabled = m_iconsEnabled;
    config.iconSet = m_iconSet;
    config.fontFile = m_fontFile;
    config.fontSize = m_fontSize;
    config.imguiStyleData = m_styleData;
}

void Menu_Appearance_State::Save() noexcept
{
    Devils_Den_Config config{};
    CopyToConfig(config);
    std::string error;
    if (Devils_Den_Config_Store::SaveAppearance(config, &error)) {
        LogInfo("Saved last-session appearance to " + Devils_Den_Config_Store::SettingsPath().string(),
            "Appearance.Config");
    } else {
        m_status = error;
        LogError("Appearance save failed: " + error, "Appearance.Config");
    }
}

void Menu_Appearance_State::ApplyFromConfig(const Devils_Den_Config& config) noexcept
{
    m_theme = std::clamp(config.menuTheme, 0, Themes::Menu_Theme_Count - 1);
    m_bannerEnabled = config.bannerEnabled;
    m_bannerImage = config.bannerImage;
    m_bannerOpacity = std::clamp(config.bannerOpacity, 0.10F, 1.0F);
    m_backgroundEnabled = config.backgroundEnabled;
    m_backgroundImage = config.backgroundImage;
    m_backgroundOpacity = std::clamp(config.backgroundOpacity, 0.0F, 1.0F);
    m_backgroundFit = std::clamp(config.backgroundFit, 0, 2);
    m_iconsEnabled = config.iconsEnabled;
    m_iconSet = config.iconSet;
    m_fontFile = config.fontFile;
    m_fontSize = std::clamp(config.fontSize, 10.0F, 42.0F);
    m_styleData = config.imguiStyleData;
    RefreshImages();
    ApplyThemeAndStyle();
    if (!m_fontFile.empty() && HasFont(m_fontFile))
        (void)ApplySelectedFont(false);
    else
        ResetFont(false);
    Save();
    m_status = "Named config appearance applied.";
    LogInfo("Applied appearance from named config: " + config.name, "Appearance.Config");
}

void Menu_Appearance_State::ReloadAll() noexcept
{
    Devils_Den_Config config{};
    std::string error;
    if (!Devils_Den_Config_Store::LoadAppearance(config, &error)) {
        m_status = error;
        LogWarning("Appearance reload failed: " + error, "Appearance.Config");
        return;
    }
    ApplyFromConfig(config);
    m_status = "Reloaded all appearance settings from Settings.json.";
}

void Menu_Appearance_State::LogInfo(const std::string& message, const char* service) noexcept
{
    if (m_logger)
        m_logger->Log(Backend::LogLevel::Info, message, service ? service : "Appearance");
}

void Menu_Appearance_State::LogWarning(const std::string& message, const char* service) noexcept
{
    if (m_logger)
        m_logger->Log(Backend::LogLevel::Warning, message, service ? service : "Appearance");
}

void Menu_Appearance_State::LogError(const std::string& message, const char* service) noexcept
{
    if (m_logger)
        m_logger->Log(Backend::LogLevel::Error, message, service ? service : "Appearance");
}
}
