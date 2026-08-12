#include "Frontend/Menu/Menu_Appearance.hpp"

#include <Windows.h>
#include <shobjidl.h>
#include <utility>

namespace Devilz::Frontend
{
namespace
{
std::string BrowseBannerImage()
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
        (void)dialog->SetTitle(L"Select Devil's Den Banner Image");
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
}

void Devils_Den_Menu::DrawSettingsPage()
{
    auto& gameplay = GTA_Gameplay_State::Instance();
    auto& vehicles = GTA_Vehicle_State::Instance();
    auto& appearance = Menu_Appearance_State::Instance();
    appearance.InitializeOnce();

    static bool configsLoaded = false;
    static std::vector<Devils_Den_Config> configs;
    static int selectedConfig = -1;
    static std::array<char, 64> configName{};
    static std::array<char, 512> bannerPath{};
    static bool seeded = false;
    static std::string configStatus;

    if (!seeded) {
        CopyToBuffer("Default", configName.data(), configName.size());
        CopyToBuffer(appearance.BannerPath(), bannerPath.data(), bannerPath.size());
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
    ImGui::TextDisabled("- appearance, banner image and configuration");
    MedievalDivider();

    ImGui::TextColored(Bronze, "THEMES & BANNER");
    int themeIndex = appearance.Theme();
    ImGui::SetNextItemWidth(280.0F);
    if (ImGui::BeginCombo("Theme", Themes::MenuThemeName(static_cast<Themes::Menu_Theme_Id>(themeIndex)))) {
        for (int index = 0; index < Themes::Menu_Theme_Count; ++index) {
            const bool selected = index == themeIndex;
            if (ImGui::Selectable(Themes::MenuThemeName(static_cast<Themes::Menu_Theme_Id>(index)), selected)) {
                themeIndex = index;
                appearance.SetTheme(index);
            }
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    bool bannerEnabled = appearance.BannerEnabled();
    if (ImGui::Checkbox("Use Banner Image", &bannerEnabled))
        appearance.SetBannerEnabled(bannerEnabled);

    ImGui::SetNextItemWidth(520.0F);
    ImGui::InputTextWithHint("##BannerPath", "PNG / JPG / BMP image path...", bannerPath.data(), bannerPath.size());
    ImGui::SameLine();
    if (ImGui::Button("BROWSE", ImVec2(100.0F, 34.0F))) {
        const auto selected = BrowseBannerImage();
        if (!selected.empty()) {
            CopyToBuffer(selected, bannerPath.data(), bannerPath.size());
            appearance.LoadBanner(selected);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("LOAD BANNER", ImVec2(130.0F, 34.0F)))
        appearance.LoadBanner(bannerPath.data());
    ImGui::SameLine();
    if (ImGui::Button("CLEAR", ImVec2(85.0F, 34.0F))) {
        bannerPath.fill('\0');
        appearance.ClearBanner();
    }

    float opacity = appearance.BannerOpacity();
    ImGui::SetNextItemWidth(280.0F);
    if (ImGui::SliderFloat("Banner Opacity", &opacity, 0.10F, 1.0F, "%.2f", ImGuiSliderFlags_AlwaysClamp))
        appearance.SetBannerOpacity(opacity);

    ImGui::TextDisabled("Settings file: %s", Devils_Den_Config_Store::SettingsPath().string().c_str());
    if (!appearance.Status().empty())
        ImGui::TextWrapped("Banner: %s", appearance.Status().c_str());

    MedievalDivider();
    ImGui::TextColored(Bronze, "CONFIGS");
    ImGui::TextDisabled("Folder: %s", Devils_Den_Config_Store::RootDirectory().string().c_str());
    ImGui::SetNextItemWidth(280.0F);
    ImGui::InputTextWithHint("##ConfigName", "Config name...", configName.data(), configName.size());
    ImGui::SameLine();
    if (ImGui::Button("SAVE CONFIG", ImVec2(130.0F, 34.0F))) {
        Devils_Den_Config config{};
        config.name = configName.data();
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
            configStatus = "Saved config JSON with appearance settings.";
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
    if (ImGui::BeginChild("##ConfigList", ImVec2(280.0F, 300.0F), ImGuiChildFlags_Borders)) {
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

    if (ImGui::BeginChild("##ConfigDetails", ImVec2(0.0F, 300.0F), ImGuiChildFlags_Borders)) {
        if (selectedConfig >= 0 && selectedConfig < static_cast<int>(configs.size())) {
            const auto& config = configs[static_cast<std::size_t>(selectedConfig)];
            ImGui::TextColored(Bronze, "%s", config.name.c_str());
            ImGui::Text("Theme: %s", Themes::MenuThemeName(static_cast<Themes::Menu_Theme_Id>(std::clamp(config.menuTheme, 0, Themes::Menu_Theme_Count - 1))));
            ImGui::Text("Banner: %s", config.bannerEnabled ? "ON" : "OFF");
            ImGui::Text("Infinite Ammo: %s", config.infiniteAmmo ? "ON" : "OFF");
            ImGui::Text("Explosive Bullets: %s", config.explosiveBullets ? "ON" : "OFF");
            ImGui::Text("Vehicle God Mode: %s", config.vehicleGodMode ? "ON" : "OFF");
            ImGui::Separator();

            if (ImGui::Button("LOAD CONFIG", ImVec2(140.0F, 36.0F))) {
                appearance.ApplyFromConfig(config);
                CopyToBuffer(appearance.BannerPath(), bannerPath.data(), bannerPath.size());
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
                configStatus = "Loaded config and applied appearance/live settings.";
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
    ImGui::TextDisabled("Settings.json auto-loads theme/banner on startup. Named configs also persist appearance and live menu settings.");
}
}
