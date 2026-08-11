#pragma once

#include "GTA_Vehicle_State.hpp"

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
    int version = 1;
    std::string name;
    std::uint32_t modelHash = 0;
    std::string modelName;
    std::string displayName;
    std::string makeName;
    int wheelType = -1;
    int windowTint = -1;
    int plateStyle = -1;
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
