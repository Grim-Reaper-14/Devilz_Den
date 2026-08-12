#include "Frontend/Menu/Menu_Appearance.hpp"

#include <Windows.h>
#include <shellapi.h>
#include <shobjidl.h>

#include <array>
#include <string>
#include <utility>

namespace Devilz::Frontend
{
namespace
{
std::string BrowseAppearanceImage()
{
    const HRESULT initHr = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool uninit = SUCCEEDED(initHr);
    if (FAILED(initHr) && initHr != RPC_E_CHANGED_MODE)
        return {};

    IFileOpenDialog* dialog = nullptr;
    std::string result;
    if (SUCCEEDED(::CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&dialog))) && dialog) {
        const COMDLG_FILTERSPEC filters[] = {
            {L"Image Files", L"*.png;*.jpg;*.jpeg;*.bmp;*.gif;*.tif;*.tiff"},
            {L"All Files", L"*.*"}
        };
        (void)dialog->SetFileTypes(2, filters);
        (void)dialog->SetTitle(L"Import Devil's Den Appearance Image");
        if (SUCCEEDED(dialog->Show(nullptr))) {
            IShellItem* item = nullptr;
            if (SUCCEEDED(dialog->GetResult(&item)) && item) {
                PWSTR path = nullptr;
                if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path)) && path) {
                    const int count = ::WideCharToMultiByte(CP_UTF8, 0, path, -1, nullptr, 0, nullptr, nullptr);
                    if (count > 1) {
                        std::string buffer(static_cast<std::size_t>(count), '\0');
                        (void)::WideCharToMultiByte(CP_UTF8, 0, path, -1, buffer.data(), count, nullptr, nullptr);
                        buffer.resize(static_cast<std::size_t>(count - 1));
                        result = std::move(buffer);
                    }
                    ::CoTaskMemFree(path);
                }
                item->Release();
            }
        }
        dialog->Release();
    }
    if (uninit) ::CoUninitialize();
    return result;
}

void OpenAppearanceFolder(const std::filesystem::path& folder)
{
    const auto path = folder.wstring();
    (void)::ShellExecuteW(nullptr, L"open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

bool DrawImageTile(
    Menu_Appearance_State& appearance,
    const Appearance_Image_Asset& asset,
    bool selected,
    float width = 170.0F,
    float height = 80.0F)
{
    bool clicked = false;
    ImGui::PushID(asset.name.c_str());
    const auto view = appearance.ImageView(asset);
    if (view.ready) {
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
        ImGui::Image(view.texture, ImVec2(width, height), uv0, uv1);
        clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
    } else {
        ImGui::Button("IMAGE LOAD FAILED", ImVec2(width, height));
        clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
    }
    if (selected)
        ImGui::TextColored(EmberRed, "SELECTED");
    else
        ImGui::TextDisabled("click to select");
    ImGui::TextWrapped("%s", asset.name.c_str());
    ImGui::PopID();
    return clicked;
}
}

void Devils_Den_Menu::DrawSettingsPage()
{
    auto& gameplay = GTA_Gameplay_State::Instance();
    auto& vehicles = GTA_Vehicle_State::Instance();
    auto& appearance = Menu_Appearance_State::Instance();
    appearance.InitializeOnce();
    appearance.Tick();

    static bool configsLoaded = false;
    static std::vector<Devils_Den_Config> configs;
    static int selectedConfig = -1;
    static std::array<char, 64> configName{};
    static std::array<char, 128> fontSearch{};
    static std::string configStatus;
    static bool seeded = false;

    if (!seeded) {
        CopyToBuffer("Default", configName.data(), configName.size());
        seeded = true;
    }

    const auto reloadConfigs = [&]() {
        configs = Devils_Den_Config_Store::LoadAll();
        if (configs.empty()) selectedConfig = -1;
        else selectedConfig = std::clamp(selectedConfig, 0, static_cast<int>(configs.size()) - 1);
        configsLoaded = true;
    };
    if (!configsLoaded) reloadConfigs();

    ImGui::TextColored(EmberRed, "SETTINGS");
    ImGui::SameLine();
    ImGui::TextDisabled("- themes, images, icons, fonts, style and configs");
    MedievalDivider();

    if (ImGui::BeginTabBar("##AppearanceSettingsTabs")) {
        if (ImGui::BeginTabItem("APPEARANCE")) {
            ImGui::TextColored(Bronze, "THEME & SESSION");
            int themeIndex = appearance.Theme();
            ImGui::SetNextItemWidth(320.0F);
            if (ImGui::BeginCombo("Theme", Themes::MenuThemeName(static_cast<Themes::Menu_Theme_Id>(themeIndex)))) {
                for (int index = 0; index < Themes::Menu_Theme_Count; ++index) {
                    const bool selected = index == themeIndex;
                    if (ImGui::Selectable(Themes::MenuThemeName(static_cast<Themes::Menu_Theme_Id>(index)), selected))
                        appearance.SetTheme(index);
                    if (selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            if (ImGui::Button("SAVE ALL", ImVec2(120.0F, 34.0F))) appearance.Save();
            ImGui::SameLine();
            if (ImGui::Button("RELOAD ALL", ImVec2(130.0F, 34.0F))) appearance.ReloadAll();
            ImGui::SameLine();
            if (ImGui::Button("OPEN DEVILZ_DEN", ImVec2(170.0F, 34.0F)))
                OpenAppearanceFolder(Menu_Appearance_State::RootDirectory());

            ImGui::Spacing();
            ImGui::TextDisabled("Last-session settings: %s", Devils_Den_Config_Store::SettingsPath().string().c_str());
            ImGui::TextDisabled("Managed images: %s", Menu_Appearance_State::ImagesDirectory().string().c_str());
            ImGui::TextWrapped("Status: %s", appearance.Status().c_str());
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("IMAGES")) {
            ImGui::TextColored(Bronze, "MANAGED IMAGE LOADER");
            ImGui::TextWrapped("Images are loaded from Devilz_Den\\Images. The same cached GPU texture is used for the preview and the live menu.");
            if (ImGui::Button("OPEN IMAGES FOLDER", ImVec2(175.0F, 34.0F)))
                OpenAppearanceFolder(Menu_Appearance_State::ImagesDirectory());
            ImGui::SameLine();
            if (ImGui::Button("REFRESH IMAGES", ImVec2(150.0F, 34.0F))) appearance.RefreshImages();

            MedievalDivider();
            ImGui::TextColored(Bronze, "HEADER BANNERS");
            bool bannerEnabled = appearance.BannerEnabled();
            if (ImGui::Checkbox("Use Header Banner", &bannerEnabled)) appearance.SetBannerEnabled(bannerEnabled);
            float bannerOpacity = appearance.BannerOpacity();
            ImGui::SetNextItemWidth(260.0F);
            if (ImGui::SliderFloat("Banner Opacity", &bannerOpacity, 0.10F, 1.0F, "%.2f", ImGuiSliderFlags_AlwaysClamp))
                appearance.SetBannerOpacity(bannerOpacity);
            if (ImGui::Button("IMPORT BANNER", ImVec2(145.0F, 32.0F))) {
                const auto source = BrowseAppearanceImage();
                if (!source.empty()) {
                    std::string imported;
                    std::string error;
                    if (appearance.ImportImage(Appearance_Image_Category::Banner, source, &imported, &error))
                        appearance.SelectBanner(imported);
                    else
                        configStatus = error;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("OPEN BANNERS", ImVec2(135.0F, 32.0F)))
                OpenAppearanceFolder(Menu_Appearance_State::BannersDirectory());

            if (ImGui::BeginChild("##BannerPreviewGrid", ImVec2(0.0F, 210.0F), ImGuiChildFlags_Borders)) {
                int column = 0;
                for (const auto& asset : appearance.Banners()) {
                    if (column != 0) ImGui::SameLine();
                    ImGui::BeginGroup();
                    if (DrawImageTile(appearance, asset, appearance.BannerImage() == asset.name))
                        appearance.SelectBanner(asset.name);
                    ImGui::EndGroup();
                    column = (column + 1) % 3;
                }
                if (appearance.Banners().empty())
                    ImGui::TextDisabled("No banner images. Put files in Images\\Banners or use IMPORT BANNER.");
            }
            ImGui::EndChild();

            MedievalDivider();
            ImGui::TextColored(Bronze, "MENU BACKGROUND");
            bool backgroundEnabled = appearance.BackgroundEnabled();
            if (ImGui::Checkbox("Use Menu Background", &backgroundEnabled)) appearance.SetBackgroundEnabled(backgroundEnabled);
            float backgroundOpacity = appearance.BackgroundOpacity();
            ImGui::SetNextItemWidth(260.0F);
            if (ImGui::SliderFloat("Background Opacity", &backgroundOpacity, 0.0F, 1.0F, "%.2f", ImGuiSliderFlags_AlwaysClamp))
                appearance.SetBackgroundOpacity(backgroundOpacity);
            int fit = appearance.BackgroundFit();
            static constexpr const char* fitNames[] = {"Fill / Crop", "Fit", "Stretch"};
            ImGui::SetNextItemWidth(220.0F);
            if (ImGui::Combo("Background Fit", &fit, fitNames, 3)) appearance.SetBackgroundFit(fit);
            if (ImGui::Button("IMPORT BACKGROUND", ImVec2(170.0F, 32.0F))) {
                const auto source = BrowseAppearanceImage();
                if (!source.empty()) {
                    std::string imported;
                    std::string error;
                    if (appearance.ImportImage(Appearance_Image_Category::Background, source, &imported, &error))
                        appearance.SelectBackground(imported);
                    else
                        configStatus = error;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("OPEN BACKGROUNDS", ImVec2(165.0F, 32.0F)))
                OpenAppearanceFolder(Menu_Appearance_State::BackgroundsDirectory());

            if (ImGui::BeginChild("##BackgroundPreviewGrid", ImVec2(0.0F, 210.0F), ImGuiChildFlags_Borders)) {
                int column = 0;
                for (const auto& asset : appearance.Backgrounds()) {
                    if (column != 0) ImGui::SameLine();
                    ImGui::BeginGroup();
                    if (DrawImageTile(appearance, asset, appearance.BackgroundImage() == asset.name))
                        appearance.SelectBackground(asset.name);
                    ImGui::EndGroup();
                    column = (column + 1) % 3;
                }
                if (appearance.Backgrounds().empty())
                    ImGui::TextDisabled("No background images. Put files in Images\\Backgrounds or use IMPORT BACKGROUND.");
            }
            ImGui::EndChild();

            MedievalDivider();
            ImGui::TextColored(Bronze, "ICON SETS");
            ImGui::TextWrapped("Create folders under Images\\Icons. Use filenames: devilcrest, self, weapons, vehicle, teleport, world, network, lua, settings (PNG/JPG/BMP etc.).");
            bool iconsEnabled = appearance.IconsEnabled();
            if (ImGui::Checkbox("Use Image Icons", &iconsEnabled)) appearance.SetIconsEnabled(iconsEnabled);
            if (ImGui::Button("OPEN ICONS", ImVec2(130.0F, 32.0F)))
                OpenAppearanceFolder(Menu_Appearance_State::IconsDirectory());
            ImGui::SameLine();
            if (ImGui::Button("REFRESH ICON SETS", ImVec2(165.0F, 32.0F))) appearance.RefreshImages();

            if (ImGui::BeginChild("##IconSets", ImVec2(0.0F, 155.0F), ImGuiChildFlags_Borders)) {
                for (const auto& set : appearance.IconSets()) {
                    ImGui::PushID(set.c_str());
                    const bool selected = set == appearance.IconSet();
                    const auto previewPath = appearance.IconSetPreviewPath(set);
                    if (!previewPath.empty()) {
                        const auto preview = Renderer::D3D12_Image_Loader::Instance().View(previewPath);
                        if (preview.ready) {
                            ImGui::Image(preview.texture, ImVec2(48.0F, 48.0F));
                            if (ImGui::IsItemClicked()) appearance.SelectIconSet(set);
                            ImGui::SameLine();
                        }
                    }
                    if (ImGui::Selectable(set.c_str(), selected, 0, ImVec2(220.0F, 48.0F)))
                        appearance.SelectIconSet(set);
                    ImGui::PopID();
                }
                if (appearance.IconSets().empty()) ImGui::TextDisabled("No icon sets found.");
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("FONTS")) {
            ImGui::TextColored(Bronze, "WINDOWS FONT LOADER");
            ImGui::TextDisabled("Folder: %s", Menu_Appearance_State::WindowsFontsDirectory().string().c_str());
            if (ImGui::Button("REFRESH FONTS", ImVec2(145.0F, 32.0F))) appearance.RefreshFonts();
            ImGui::SameLine();
            if (ImGui::Button("OPEN WINDOWS FONTS", ImVec2(180.0F, 32.0F)))
                OpenAppearanceFolder(Menu_Appearance_State::WindowsFontsDirectory());

            ImGui::SetNextItemWidth(360.0F);
            ImGui::InputTextWithHint("##FontSearch", "Search fonts...", fontSearch.data(), fontSearch.size());
            float fontSize = appearance.FontSize();
            ImGui::SetNextItemWidth(240.0F);
            if (ImGui::SliderFloat("Font Size", &fontSize, 10.0F, 42.0F, "%.1f px", ImGuiSliderFlags_AlwaysClamp))
                appearance.SetFontSize(fontSize);

            if (ImGui::Button("APPLY FONT", ImVec2(130.0F, 32.0F))) (void)appearance.ApplySelectedFont(true);
            ImGui::SameLine();
            if (ImGui::Button("RESET DEFAULT", ImVec2(145.0F, 32.0F))) appearance.ResetFont(true);

            MedievalDivider();
            const float listWidth = 320.0F;
            if (ImGui::BeginChild("##FontList", ImVec2(listWidth, 360.0F), ImGuiChildFlags_Borders)) {
                for (const auto& font : appearance.Fonts()) {
                    if (!ContainsInsensitive(font.displayName, fontSearch.data()) &&
                        !ContainsInsensitive(font.fileName, fontSearch.data()))
                        continue;
                    const bool selected = font.fileName == appearance.SelectedFontFile();
                    if (ImGui::Selectable(font.displayName.c_str(), selected))
                        appearance.SelectFont(font.fileName);
                    if (ImGui::IsItemHovered()) {
                        if (auto* previewFont = appearance.PreviewFont(font)) {
                            ImGui::BeginTooltip();
                            ImGui::PushFont(previewFont, 18.0F);
                            ImGui::TextUnformatted("DEVIL'S DEN MENU");
                            ImGui::TextUnformatted("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
                            ImGui::TextUnformatted("abcdefghijklmnopqrstuvwxyz 0123456789");
                            ImGui::PopFont();
                            ImGui::TextDisabled("%s", font.fileName.c_str());
                            ImGui::EndTooltip();
                        }
                    }
                }
            }
            ImGui::EndChild();
            ImGui::SameLine();
            if (ImGui::BeginChild("##FontPreview", ImVec2(0.0F, 360.0F), ImGuiChildFlags_Borders)) {
                ImGui::TextColored(Bronze, "FONT PREVIEW");
                const Appearance_Font_Asset* selectedAsset = nullptr;
                for (const auto& font : appearance.Fonts()) {
                    if (font.fileName == appearance.SelectedFontFile()) {
                        selectedAsset = &font;
                        break;
                    }
                }
                if (selectedAsset) {
                    ImGui::TextDisabled("%s", selectedAsset->fileName.c_str());
                    if (auto* previewFont = appearance.PreviewFont(*selectedAsset)) {
                        ImGui::Spacing();
                        ImGui::PushFont(previewFont, fontSize);
                        ImGui::TextUnformatted("DEVIL'S DEN MENU");
                        ImGui::TextUnformatted("The Devil Made Me Do It");
                        ImGui::TextUnformatted("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
                        ImGui::TextUnformatted("abcdefghijklmnopqrstuvwxyz");
                        ImGui::TextUnformatted("0123456789 !@#$%%^&*()");
                        ImGui::PopFont();
                    } else {
                        ImGui::TextDisabled("Preview could not be loaded for this font.");
                    }
                } else {
                    ImGui::TextDisabled("Select a Windows font to preview it here.");
                }
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("IMGUI STYLE")) {
            ImGui::TextColored(Bronze, "FULL IMGUI STYLE EDITOR");
            ImGui::TextWrapped("Changes in the editor are live. APPLY captures the current editor state in memory; SAVE persists it to Settings.json.");
            if (ImGui::Button("APPLY", ImVec2(105.0F, 32.0F))) appearance.CaptureStyleFromImGui(false);
            ImGui::SameLine();
            if (ImGui::Button("SAVE", ImVec2(105.0F, 32.0F))) appearance.CaptureStyleFromImGui(true);
            ImGui::SameLine();
            if (ImGui::Button("LOAD", ImVec2(105.0F, 32.0F))) appearance.ApplyStoredStyle();
            ImGui::SameLine();
            if (ImGui::Button("RELOAD", ImVec2(105.0F, 32.0F))) appearance.ReloadStyleFromSettings();
            ImGui::SameLine();
            if (ImGui::Button("RESET THEME", ImVec2(135.0F, 32.0F))) appearance.ResetStyleToTheme(true);
            MedievalDivider();
            if (ImGui::BeginChild("##StyleEditor", ImVec2(0.0F, 560.0F), ImGuiChildFlags_Borders))
                ImGui::ShowStyleEditor(&appearance.StyleReference());
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("CONFIGS")) {
            ImGui::TextColored(Bronze, "NAMED CONFIGS");
            ImGui::TextWrapped("Named configs now include theme, banner, background, icon set, font, complete ImGui style, and live gameplay/vehicle settings.");
            ImGui::SetNextItemWidth(280.0F);
            ImGui::InputTextWithHint("##ConfigName", "Config name...", configName.data(), configName.size());
            ImGui::SameLine();
            if (ImGui::Button("SAVE CONFIG", ImVec2(130.0F, 34.0F))) {
                Devils_Den_Config config{};
                config.name = configName.data();
                appearance.CaptureStyleFromImGui(false);
                appearance.CopyToConfig(config);
                config.godMode = gameplay.GodMode();
                config.neverWanted = gameplay.NeverWanted();
                config.superJump = gameplay.SuperJump();
                config.infiniteOxygen = gameplay.InfiniteOxygen();
                config.noRagdoll = gameplay.NoRagdoll();
                config.keepPlayerClean = gameplay.KeepPlayerClean();
                config.infiniteAmmo = gameplay.InfiniteAmmo();
                config.unlimitedClip = gameplay.UnlimitedClip();
                config.explosiveBullets = gameplay.ExplosiveBullets();
                config.explosionType = gameplay.ExplosionType();
                config.explosionDamageScale = gameplay.ExplosionDamageScale();
                config.explosionCameraShake = gameplay.ExplosionCameraShake();
                config.keepVehiclePerfect = vehicles.KeepVehiclePerfect();
                config.vehicleGodMode = vehicles.VehicleGodMode();
                config.spawnInsideVehicle = m_spawnInsideVehicle;
                config.spawnVehicleMaxed = m_spawnVehicleMaxed;
                config.spawnVehicleOnGround = m_spawnVehicleOnGround;
                config.spawnVehicleEngineRunning = m_spawnVehicleEngineRunning;
                config.spawnVehicleInvincible = m_spawnVehicleInvincible;
                config.spawnVehicleClean = m_spawnVehicleClean;

                std::string error;
                if (Devils_Den_Config_Store::Save(config, &error)) {
                    configStatus = "Saved config with complete appearance + live settings.";
                    reloadConfigs();
                } else configStatus = error.empty() ? "Saving config failed." : error;
            }
            ImGui::SameLine();
            if (ImGui::Button("RELOAD", ImVec2(95.0F, 34.0F))) {
                reloadConfigs();
                configStatus = "Reloaded config JSON files.";
            }
            if (!configStatus.empty()) ImGui::TextWrapped("%s", configStatus.c_str());

            MedievalDivider();
            if (ImGui::BeginChild("##ConfigList", ImVec2(280.0F, 360.0F), ImGuiChildFlags_Borders)) {
                ImGui::TextColored(Bronze, "SAVED CONFIGS (%zu)", configs.size());
                for (std::size_t index = 0; index < configs.size(); ++index) {
                    ImGui::PushID(static_cast<int>(index));
                    if (ImGui::Selectable(configs[index].name.c_str(), selectedConfig == static_cast<int>(index))) {
                        selectedConfig = static_cast<int>(index);
                        CopyToBuffer(configs[index].name, configName.data(), configName.size());
                    }
                    ImGui::PopID();
                }
                if (configs.empty()) ImGui::TextDisabled("No configs saved yet.");
            }
            ImGui::EndChild();
            ImGui::SameLine();

            if (ImGui::BeginChild("##ConfigDetails", ImVec2(0.0F, 360.0F), ImGuiChildFlags_Borders)) {
                if (selectedConfig >= 0 && selectedConfig < static_cast<int>(configs.size())) {
                    const auto& config = configs[static_cast<std::size_t>(selectedConfig)];
                    ImGui::TextColored(Bronze, "%s", config.name.c_str());
                    ImGui::Text("Theme: %s", Themes::MenuThemeName(static_cast<Themes::Menu_Theme_Id>(std::clamp(config.menuTheme, 0, Themes::Menu_Theme_Count - 1))));
                    ImGui::Text("Banner: %s", config.bannerImage.empty() ? "None" : config.bannerImage.c_str());
                    ImGui::Text("Background: %s", config.backgroundImage.empty() ? "None" : config.backgroundImage.c_str());
                    ImGui::Text("Icons: %s", config.iconSet.empty() ? "Procedural" : config.iconSet.c_str());
                    ImGui::Text("Font: %s @ %.1f", config.fontFile.empty() ? "Default" : config.fontFile.c_str(), config.fontSize);
                    ImGui::Separator();

                    if (ImGui::Button("LOAD CONFIG", ImVec2(140.0F, 36.0F))) {
                        appearance.ApplyFromConfig(config);
                        gameplay.SetGodMode(config.godMode);
                        gameplay.SetNeverWanted(config.neverWanted);
                        gameplay.SetSuperJump(config.superJump);
                        gameplay.SetInfiniteOxygen(config.infiniteOxygen);
                        gameplay.SetNoRagdoll(config.noRagdoll);
                        gameplay.SetKeepPlayerClean(config.keepPlayerClean);
                        gameplay.SetInfiniteAmmo(config.infiniteAmmo);
                        gameplay.SetUnlimitedClip(config.unlimitedClip);
                        gameplay.SetExplosiveBullets(config.explosiveBullets);
                        gameplay.SetExplosionType(config.explosionType);
                        gameplay.SetExplosionDamageScale(config.explosionDamageScale);
                        gameplay.SetExplosionCameraShake(config.explosionCameraShake);
                        vehicles.SetKeepVehiclePerfect(config.keepVehiclePerfect);
                        vehicles.SetVehicleGodMode(config.vehicleGodMode);
                        m_spawnInsideVehicle = config.spawnInsideVehicle;
                        m_spawnVehicleMaxed = config.spawnVehicleMaxed;
                        m_spawnVehicleOnGround = config.spawnVehicleOnGround;
                        m_spawnVehicleEngineRunning = config.spawnVehicleEngineRunning;
                        m_spawnVehicleInvincible = config.spawnVehicleInvincible;
                        m_spawnVehicleClean = config.spawnVehicleClean;
                        configStatus = "Loaded config and restored complete appearance/live settings.";
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("DELETE CONFIG", ImVec2(145.0F, 36.0F))) {
                        std::string error;
                        if (Devils_Den_Config_Store::Remove(config, &error)) {
                            configStatus = "Deleted config JSON.";
                            reloadConfigs();
                        } else configStatus = error.empty() ? "Delete failed." : error;
                    }
                } else ImGui::TextDisabled("Select a config to inspect, load, or delete it.");
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}
}
