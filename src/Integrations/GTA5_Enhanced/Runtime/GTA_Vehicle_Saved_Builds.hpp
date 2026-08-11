#pragma once

#include "GTA_Vehicle_State.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace Devilz::Integrations::GTA5_Enhanced
{
struct GTA_Vehicle_Saved_Mod
{
    int slot = -1;
    int index = -1;
    bool customTires = false;
    std::string categoryName;
    std::string optionName;
};

struct GTA_Vehicle_Saved_Build
{
    int version = 3;
    std::string name;
    std::uint32_t modelHash = 0;
    std::string modelName;
    std::string displayName;
    std::string makeName;

    int wheelType = -1;
    std::string wheelTypeName;
    int windowTint = -1;
    std::string windowTintName;
    int plateStyle = -1;
    std::string plateStyleName;
    std::string plateText;

    int primaryPaintType = -1;
    std::string primaryPaintTypeName;
    int primaryColor = -1;
    std::string primaryColorName;
    int secondaryPaintType = -1;
    std::string secondaryPaintTypeName;
    int secondaryColor = -1;
    std::string secondaryColorName;
    int pearlescentColor = -1;
    std::string pearlescentColorName;
    int wheelColor = -1;
    std::string wheelColorName;
    bool primaryCustom = false;
    bool secondaryCustom = false;
    std::array<int, 3> primaryRgb{0, 0, 0};
    std::array<int, 3> secondaryRgb{0, 0, 0};

    bool turboEnabled = false;
    bool xenonEnabled = false;
    int xenonColor = -1;
    bool tireSmokeEnabled = false;
    std::array<int, 3> tyreSmokeRgb{255, 255, 255};
    bool tyresCanBurst = true;
    bool driftTyres = false;

    std::array<bool, 4> neonEnabled{false, false, false, false};
    std::array<int, 3> neonRgb{255, 255, 255};

    std::array<bool, 15> extraExists{};
    std::array<bool, 15> extrasEnabled{};
    int livery = -1;
    int interiorColor = -1;
    int dashboardColor = -1;

    std::vector<GTA_Vehicle_Saved_Mod> mods;
    std::filesystem::path sourcePath;
};

class GTA_Vehicle_Saved_Builds final
{
public:
    [[nodiscard]] static std::filesystem::path RootDirectory();
    [[nodiscard]] static GTA_Vehicle_Saved_Build Capture(
        std::string name,
        const GTA_Vehicle_Metadata* metadata,
        const GTA_Vehicle_Forge_Snapshot& snapshot);
    [[nodiscard]] static std::vector<GTA_Vehicle_Saved_Build> LoadAll();
    [[nodiscard]] static bool Save(const GTA_Vehicle_Saved_Build& build, std::string* error = nullptr);
    [[nodiscard]] static bool Remove(const GTA_Vehicle_Saved_Build& build, std::string* error = nullptr);
    [[nodiscard]] static std::vector<GTA_Vehicle_Forge_Command> BuildCommands(
        const GTA_Vehicle_Saved_Build& build);
    static void QueueApply(const GTA_Vehicle_Saved_Build& build, GTA_Vehicle_State& state);
};
}
