#include "GTA_Vehicle_Saved_Builds.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string_view>

namespace Devilz::Integrations::GTA5_Enhanced
{
namespace
{
std::string EscapeJson(std::string_view value)
{
    std::string out;
    out.reserve(value.size() + 8U);
    for (const char c : value) {
        switch (c) {
        case '\\': out += "\\\\"; break;
        case '"': out += "\\\""; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default: out.push_back(c); break;
        }
    }
    return out;
}

std::string SafeFilename(std::string_view value)
{
    std::string out;
    out.reserve(value.size());
    for (const unsigned char c : value) {
        if (std::isalnum(c) || c == '-' || c == '_')
            out.push_back(static_cast<char>(c));
        else if (std::isspace(c))
            out.push_back('_');
    }
    while (!out.empty() && out.back() == '_')
        out.pop_back();
    return out.empty() ? "vehicle" : out;
}

std::size_t ValueStart(std::string_view text, std::string_view key)
{
    const std::string needle = "\"" + std::string(key) + "\"";
    const auto keyPos = text.find(needle);
    if (keyPos == std::string_view::npos)
        return std::string_view::npos;
    const auto colon = text.find(':', keyPos + needle.size());
    if (colon == std::string_view::npos)
        return std::string_view::npos;
    auto pos = colon + 1U;
    while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos])))
        ++pos;
    return pos;
}

bool ReadString(std::string_view text, std::string_view key, std::string& output)
{
    auto pos = ValueStart(text, key);
    if (pos == std::string_view::npos || pos >= text.size() || text[pos] != '"')
        return false;
    ++pos;
    output.clear();
    bool escaped = false;
    for (; pos < text.size(); ++pos) {
        const char c = text[pos];
        if (escaped) {
            switch (c) {
            case 'n': output.push_back('\n'); break;
            case 'r': output.push_back('\r'); break;
            case 't': output.push_back('\t'); break;
            default: output.push_back(c); break;
            }
            escaped = false;
            continue;
        }
        if (c == '\\') {
            escaped = true;
            continue;
        }
        if (c == '"')
            return true;
        output.push_back(c);
    }
    return false;
}

template <typename Integer>
bool ReadInteger(std::string_view text, std::string_view key, Integer& output)
{
    const auto pos = ValueStart(text, key);
    if (pos == std::string_view::npos)
        return false;
    try {
        std::size_t used = 0;
        const auto value = std::stoll(std::string(text.substr(pos)), &used, 10);
        if (used == 0)
            return false;
        output = static_cast<Integer>(value);
        return true;
    } catch (...) {
        return false;
    }
}

bool ReadBool(std::string_view text, std::string_view key, bool& output)
{
    const auto pos = ValueStart(text, key);
    if (pos == std::string_view::npos)
        return false;
    if (text.substr(pos, 4U) == "true") {
        output = true;
        return true;
    }
    if (text.substr(pos, 5U) == "false") {
        output = false;
        return true;
    }
    return false;
}

bool ParseBuild(std::string_view text, GTA_Vehicle_Saved_Build& build)
{
    if (!ReadString(text, "name", build.name))
        return false;
    (void)ReadInteger(text, "version", build.version);
    (void)ReadInteger(text, "modelHash", build.modelHash);
    (void)ReadString(text, "modelName", build.modelName);
    (void)ReadString(text, "displayName", build.displayName);
    (void)ReadString(text, "makeName", build.makeName);
    (void)ReadInteger(text, "wheelType", build.wheelType);
    (void)ReadInteger(text, "windowTint", build.windowTint);
    (void)ReadInteger(text, "plateStyle", build.plateStyle);

    const auto modsKey = text.find("\"mods\"");
    if (modsKey == std::string_view::npos)
        return build.modelHash != 0;
    const auto arrayStart = text.find('[', modsKey);
    const auto arrayEnd = text.find(']', arrayStart);
    if (arrayStart == std::string_view::npos || arrayEnd == std::string_view::npos)
        return false;

    std::size_t cursor = arrayStart + 1U;
    while (cursor < arrayEnd) {
        const auto objectStart = text.find('{', cursor);
        if (objectStart == std::string_view::npos || objectStart >= arrayEnd)
            break;
        const auto objectEnd = text.find('}', objectStart);
        if (objectEnd == std::string_view::npos || objectEnd > arrayEnd)
            return false;
        const auto object = text.substr(objectStart, objectEnd - objectStart + 1U);
        GTA_Vehicle_Saved_Mod mod{};
        (void)ReadInteger(object, "slot", mod.slot);
        (void)ReadInteger(object, "index", mod.index);
        (void)ReadBool(object, "customTires", mod.customTires);
        (void)ReadString(object, "categoryName", mod.categoryName);
        (void)ReadString(object, "optionName", mod.optionName);
        if (mod.slot >= 0)
            build.mods.push_back(std::move(mod));
        cursor = objectEnd + 1U;
    }
    return build.modelHash != 0;
}
}

std::filesystem::path GTA_Vehicle_Saved_Builds::RootDirectory()
{
    if (const char* localAppData = std::getenv("LOCALAPPDATA"); localAppData && *localAppData)
        return std::filesystem::path(localAppData) / "Devilz_Den" / "SavedVehicles";
    return std::filesystem::current_path() / "Devilz_Den" / "SavedVehicles";
}

GTA_Vehicle_Saved_Build GTA_Vehicle_Saved_Builds::Capture(
    std::string name,
    const GTA_Vehicle_Metadata* metadata,
    const GTA_Vehicle_Forge_Snapshot& snapshot)
{
    GTA_Vehicle_Saved_Build build{};
    build.name = std::move(name);
    build.modelHash = snapshot.modelHash;
    build.wheelType = snapshot.wheelType;
    build.windowTint = snapshot.windowTint;
    build.plateStyle = snapshot.plateStyle;

    if (metadata) {
        build.modelName = metadata->modelName;
        build.displayName = metadata->displayName;
        build.makeName = metadata->makeName;
    }

    for (const auto& category : snapshot.categories) {
        if (category.installedIndex < 0)
            continue;
        GTA_Vehicle_Saved_Mod mod{};
        mod.slot = category.slot;
        mod.index = category.installedIndex;
        mod.categoryName = category.name;
        const auto selected = std::find_if(category.options.begin(), category.options.end(),
            [&category](const GTA_Vehicle_Forge_Option& option) {
                return option.index == category.installedIndex;
            });
        if (selected != category.options.end())
            mod.optionName = selected->name;
        build.mods.push_back(std::move(mod));
    }

    return build;
}

std::vector<GTA_Vehicle_Saved_Build> GTA_Vehicle_Saved_Builds::LoadAll()
{
    std::vector<GTA_Vehicle_Saved_Build> builds;
    const auto root = RootDirectory();
    std::error_code ec;
    if (!std::filesystem::exists(root, ec))
        return builds;

    for (const auto& entry : std::filesystem::directory_iterator(root, ec)) {
        if (ec)
            break;
        if (!entry.is_regular_file() || entry.path().extension() != ".json")
            continue;
        std::ifstream stream(entry.path(), std::ios::binary);
        if (!stream)
            continue;
        std::ostringstream contents;
        contents << stream.rdbuf();
        GTA_Vehicle_Saved_Build build{};
        if (!ParseBuild(contents.str(), build))
            continue;
        build.sourcePath = entry.path();
        builds.push_back(std::move(build));
    }

    std::sort(builds.begin(), builds.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.name < rhs.name;
    });
    return builds;
}

bool GTA_Vehicle_Saved_Builds::Save(const GTA_Vehicle_Saved_Build& build, std::string* error)
{
    if (build.modelHash == 0 || build.name.empty()) {
        if (error) *error = "Vehicle build needs a name and valid model.";
        return false;
    }

    const auto root = RootDirectory();
    std::error_code ec;
    std::filesystem::create_directories(root, ec);
    if (ec) {
        if (error) *error = "Could not create SavedVehicles directory.";
        return false;
    }

    const auto path = root / (SafeFilename(build.name) + ".json");
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) {
        if (error) *error = "Could not open saved vehicle JSON for writing.";
        return false;
    }

    stream << "{\n";
    stream << "  \"version\": " << build.version << ",\n";
    stream << "  \"name\": \"" << EscapeJson(build.name) << "\",\n";
    stream << "  \"modelHash\": " << build.modelHash << ",\n";
    stream << "  \"modelName\": \"" << EscapeJson(build.modelName) << "\",\n";
    stream << "  \"displayName\": \"" << EscapeJson(build.displayName) << "\",\n";
    stream << "  \"makeName\": \"" << EscapeJson(build.makeName) << "\",\n";
    stream << "  \"wheelType\": " << build.wheelType << ",\n";
    stream << "  \"windowTint\": " << build.windowTint << ",\n";
    stream << "  \"plateStyle\": " << build.plateStyle << ",\n";
    stream << "  \"mods\": [\n";
    for (std::size_t i = 0; i < build.mods.size(); ++i) {
        const auto& mod = build.mods[i];
        stream << "    {\"slot\": " << mod.slot
               << ", \"index\": " << mod.index
               << ", \"customTires\": " << (mod.customTires ? "true" : "false")
               << ", \"categoryName\": \"" << EscapeJson(mod.categoryName)
               << "\", \"optionName\": \"" << EscapeJson(mod.optionName) << "\"}";
        if (i + 1U != build.mods.size())
            stream << ',';
        stream << '\n';
    }
    stream << "  ]\n";
    stream << "}\n";

    if (!stream.good()) {
        if (error) *error = "Writing saved vehicle JSON failed.";
        return false;
    }
    return true;
}

bool GTA_Vehicle_Saved_Builds::Remove(const GTA_Vehicle_Saved_Build& build, std::string* error)
{
    if (build.sourcePath.empty()) {
        if (error) *error = "Saved vehicle has no source file.";
        return false;
    }
    std::error_code ec;
    const bool removed = std::filesystem::remove(build.sourcePath, ec);
    if (!removed || ec) {
        if (error) *error = "Could not delete saved vehicle JSON.";
        return false;
    }
    return true;
}

std::vector<GTA_Vehicle_Forge_Command> GTA_Vehicle_Saved_Builds::BuildCommands(
    const GTA_Vehicle_Saved_Build& build)
{
    std::vector<GTA_Vehicle_Forge_Command> commands;
    commands.reserve(build.mods.size() + 4U);
    if (build.wheelType >= 0)
        commands.push_back({GTA_Vehicle_Forge_Command_Type::SetWheelType, build.wheelType, 0, 0});
    for (const auto& mod : build.mods)
        commands.push_back({GTA_Vehicle_Forge_Command_Type::SetMod, mod.slot, mod.index, mod.customTires ? 1 : 0});
    if (build.windowTint >= 0)
        commands.push_back({GTA_Vehicle_Forge_Command_Type::SetWindowTint, build.windowTint, 0, 0});
    if (build.plateStyle >= 0)
        commands.push_back({GTA_Vehicle_Forge_Command_Type::SetPlateStyle, build.plateStyle, 0, 0});
    commands.push_back({GTA_Vehicle_Forge_Command_Type::RepairVehicle, 0, 0, 0});
    return commands;
}

void GTA_Vehicle_Saved_Builds::QueueApply(const GTA_Vehicle_Saved_Build& build, GTA_Vehicle_State& state)
{
    state.QueueForgeCommands(BuildCommands(build));
}
}
