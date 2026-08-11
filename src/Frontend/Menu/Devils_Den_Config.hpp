#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace Devilz::Frontend
{
struct Devils_Den_Config
{
    int version = 1;
    std::string name;

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
    [[nodiscard]] static std::vector<Devils_Den_Config> LoadAll();
    [[nodiscard]] static bool Save(const Devils_Den_Config& config, std::string* error = nullptr);
    [[nodiscard]] static bool Remove(const Devils_Den_Config& config, std::string* error = nullptr);
};
}
