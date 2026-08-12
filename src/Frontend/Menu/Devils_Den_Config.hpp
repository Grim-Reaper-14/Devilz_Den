#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace Devilz::Frontend
{
struct Devils_Den_Config
{
    int version = 3;
    std::string name;

    int menuTheme = 0;

    bool bannerEnabled = false;
    std::string bannerImage;
    std::string bannerImagePath; // legacy v2 migration source
    float bannerOpacity = 1.0F;

    bool backgroundEnabled = false;
    std::string backgroundImage;
    float backgroundOpacity = 0.30F;
    int backgroundFit = 0; // 0 fill/crop, 1 fit, 2 stretch

    bool iconsEnabled = false;
    std::string iconSet;

    std::string fontFile;
    float fontSize = 18.0F;
    std::string imguiStyleData;

    bool godMode = false;
    bool neverWanted = false;
    bool superJump = false;
    bool infiniteOxygen = false;
    bool noRagdoll = false;
    bool keepPlayerClean = false;

    bool infiniteAmmo = false;
    bool unlimitedClip = false;
    bool explosiveBullets = false;
    int explosionType = 45;
    float explosionDamageScale = 1.0F;
    float explosionCameraShake = 0.1F;

    bool keepVehiclePerfect = false;
    bool vehicleGodMode = false;
    bool spawnInsideVehicle = true;
    bool spawnVehicleMaxed = false;
    bool spawnVehicleOnGround = true;
    bool spawnVehicleEngineRunning = true;
    bool spawnVehicleInvincible = false;
    bool spawnVehicleClean = true;

    std::filesystem::path sourcePath;
};

class Devils_Den_Config_Store final
{
public:
    [[nodiscard]] static std::filesystem::path RootDirectory();
    [[nodiscard]] static std::filesystem::path SettingsPath();
    [[nodiscard]] static std::vector<Devils_Den_Config> LoadAll();
    [[nodiscard]] static bool Save(const Devils_Den_Config& config, std::string* error = nullptr);
    [[nodiscard]] static bool Remove(const Devils_Den_Config& config, std::string* error = nullptr);
    [[nodiscard]] static bool LoadAppearance(Devils_Den_Config& config, std::string* error = nullptr);
    [[nodiscard]] static bool SaveAppearance(const Devils_Den_Config& config, std::string* error = nullptr);
};
}
