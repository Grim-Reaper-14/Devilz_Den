#include "Devils_Den_Config.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string_view>

namespace Devilz::Frontend
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
    return out.empty() ? "config" : out;
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
        if (c == '\\') { escaped = true; continue; }
        if (c == '"') return true;
        output.push_back(c);
    }
    return false;
}

bool ReadBool(std::string_view text, std::string_view key, bool& output)
{
    const auto pos = ValueStart(text, key);
    if (pos == std::string_view::npos)
        return false;
    if (text.substr(pos, 4U) == "true") { output = true; return true; }
    if (text.substr(pos, 5U) == "false") { output = false; return true; }
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
        if (used == 0) return false;
        output = static_cast<Integer>(value);
        return true;
    } catch (...) {
        return false;
    }
}

bool ReadFloat(std::string_view text, std::string_view key, float& output)
{
    const auto pos = ValueStart(text, key);
    if (pos == std::string_view::npos)
        return false;
    try {
        std::size_t used = 0;
        output = std::stof(std::string(text.substr(pos)), &used);
        return used != 0;
    } catch (...) {
        return false;
    }
}

void ParseAppearance(std::string_view text, Devils_Den_Config& config)
{
    (void)ReadInteger(text, "menuTheme", config.menuTheme);
    (void)ReadBool(text, "bannerEnabled", config.bannerEnabled);
    (void)ReadString(text, "bannerImagePath", config.bannerImagePath);
    (void)ReadFloat(text, "bannerOpacity", config.bannerOpacity);
    config.menuTheme = std::clamp(config.menuTheme, 0, 2);
    config.bannerOpacity = std::clamp(config.bannerOpacity, 0.10F, 1.0F);
}

bool ParseConfig(std::string_view text, Devils_Den_Config& config)
{
    if (!ReadString(text, "name", config.name))
        return false;
    (void)ReadInteger(text, "version", config.version);
    ParseAppearance(text, config);
    (void)ReadBool(text, "godMode", config.godMode);
    (void)ReadBool(text, "neverWanted", config.neverWanted);
    (void)ReadBool(text, "superJump", config.superJump);
    (void)ReadBool(text, "infiniteOxygen", config.infiniteOxygen);
    (void)ReadBool(text, "noRagdoll", config.noRagdoll);
    (void)ReadBool(text, "keepPlayerClean", config.keepPlayerClean);
    (void)ReadBool(text, "infiniteAmmo", config.infiniteAmmo);
    (void)ReadBool(text, "unlimitedClip", config.unlimitedClip);
    (void)ReadBool(text, "explosiveBullets", config.explosiveBullets);
    (void)ReadInteger(text, "explosionType", config.explosionType);
    (void)ReadFloat(text, "explosionDamageScale", config.explosionDamageScale);
    (void)ReadFloat(text, "explosionCameraShake", config.explosionCameraShake);
    (void)ReadBool(text, "keepVehiclePerfect", config.keepVehiclePerfect);
    (void)ReadBool(text, "vehicleGodMode", config.vehicleGodMode);
    (void)ReadBool(text, "spawnInsideVehicle", config.spawnInsideVehicle);
    (void)ReadBool(text, "spawnVehicleMaxed", config.spawnVehicleMaxed);
    (void)ReadBool(text, "spawnVehicleOnGround", config.spawnVehicleOnGround);
    (void)ReadBool(text, "spawnVehicleEngineRunning", config.spawnVehicleEngineRunning);
    (void)ReadBool(text, "spawnVehicleInvincible", config.spawnVehicleInvincible);
    (void)ReadBool(text, "spawnVehicleClean", config.spawnVehicleClean);
    return !config.name.empty();
}

bool ReadFile(const std::filesystem::path& path, std::string& text)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        return false;
    std::ostringstream contents;
    contents << stream.rdbuf();
    text = contents.str();
    return true;
}

bool EnsureRoot(std::string* error)
{
    std::error_code ec;
    std::filesystem::create_directories(Devils_Den_Config_Store::RootDirectory(), ec);
    if (ec) {
        if (error) *error = "Could not create Configs directory.";
        return false;
    }
    return true;
}

void WriteAppearance(std::ostream& stream, const Devils_Den_Config& config, bool trailingComma)
{
    const auto flag = [](bool value) { return value ? "true" : "false"; };
    stream << "  \"menuTheme\": " << std::clamp(config.menuTheme, 0, 2) << ",\n";
    stream << "  \"bannerEnabled\": " << flag(config.bannerEnabled) << ",\n";
    stream << "  \"bannerImagePath\": \"" << EscapeJson(config.bannerImagePath) << "\",\n";
    stream << "  \"bannerOpacity\": " << std::clamp(config.bannerOpacity, 0.10F, 1.0F);
    stream << (trailingComma ? ",\n" : "\n");
}
}

std::filesystem::path Devils_Den_Config_Store::RootDirectory()
{
    if (const char* localAppData = std::getenv("LOCALAPPDATA"); localAppData && *localAppData)
        return std::filesystem::path(localAppData) / "Devilz_Den" / "Configs";
    return std::filesystem::current_path() / "Devilz_Den" / "Configs";
}

std::filesystem::path Devils_Den_Config_Store::SettingsPath()
{
    return RootDirectory().parent_path() / "Settings.json";
}

std::vector<Devils_Den_Config> Devils_Den_Config_Store::LoadAll()
{
    std::vector<Devils_Den_Config> configs;
    const auto root = RootDirectory();
    std::error_code ec;
    if (!std::filesystem::exists(root, ec))
        return configs;

    for (const auto& entry : std::filesystem::directory_iterator(root, ec)) {
        if (ec) break;
        if (!entry.is_regular_file() || entry.path().extension() != ".json")
            continue;
        std::string text;
        if (!ReadFile(entry.path(), text)) continue;
        Devils_Den_Config config{};
        if (!ParseConfig(text, config)) continue;
        config.sourcePath = entry.path();
        configs.push_back(std::move(config));
    }

    std::sort(configs.begin(), configs.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.name < rhs.name;
    });
    return configs;
}

bool Devils_Den_Config_Store::Save(const Devils_Den_Config& config, std::string* error)
{
    if (config.name.empty()) {
        if (error) *error = "Config needs a name.";
        return false;
    }
    if (!EnsureRoot(error))
        return false;

    const auto path = RootDirectory() / (SafeFilename(config.name) + ".json");
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) {
        if (error) *error = "Could not open config JSON for writing.";
        return false;
    }

    const auto flag = [](bool value) { return value ? "true" : "false"; };
    stream << "{\n";
    stream << "  \"version\": 2,\n";
    stream << "  \"name\": \"" << EscapeJson(config.name) << "\",\n";
    WriteAppearance(stream, config, true);
    stream << "  \"godMode\": " << flag(config.godMode) << ",\n";
    stream << "  \"neverWanted\": " << flag(config.neverWanted) << ",\n";
    stream << "  \"superJump\": " << flag(config.superJump) << ",\n";
    stream << "  \"infiniteOxygen\": " << flag(config.infiniteOxygen) << ",\n";
    stream << "  \"noRagdoll\": " << flag(config.noRagdoll) << ",\n";
    stream << "  \"keepPlayerClean\": " << flag(config.keepPlayerClean) << ",\n";
    stream << "  \"infiniteAmmo\": " << flag(config.infiniteAmmo) << ",\n";
    stream << "  \"unlimitedClip\": " << flag(config.unlimitedClip) << ",\n";
    stream << "  \"explosiveBullets\": " << flag(config.explosiveBullets) << ",\n";
    stream << "  \"explosionType\": " << config.explosionType << ",\n";
    stream << "  \"explosionDamageScale\": " << config.explosionDamageScale << ",\n";
    stream << "  \"explosionCameraShake\": " << config.explosionCameraShake << ",\n";
    stream << "  \"keepVehiclePerfect\": " << flag(config.keepVehiclePerfect) << ",\n";
    stream << "  \"vehicleGodMode\": " << flag(config.vehicleGodMode) << ",\n";
    stream << "  \"spawnInsideVehicle\": " << flag(config.spawnInsideVehicle) << ",\n";
    stream << "  \"spawnVehicleMaxed\": " << flag(config.spawnVehicleMaxed) << ",\n";
    stream << "  \"spawnVehicleOnGround\": " << flag(config.spawnVehicleOnGround) << ",\n";
    stream << "  \"spawnVehicleEngineRunning\": " << flag(config.spawnVehicleEngineRunning) << ",\n";
    stream << "  \"spawnVehicleInvincible\": " << flag(config.spawnVehicleInvincible) << ",\n";
    stream << "  \"spawnVehicleClean\": " << flag(config.spawnVehicleClean) << "\n";
    stream << "}\n";

    if (!stream.good()) {
        if (error) *error = "Writing config JSON failed.";
        return false;
    }
    return true;
}

bool Devils_Den_Config_Store::Remove(const Devils_Den_Config& config, std::string* error)
{
    if (config.sourcePath.empty()) {
        if (error) *error = "Config has no source file.";
        return false;
    }
    std::error_code ec;
    const bool removed = std::filesystem::remove(config.sourcePath, ec);
    if (!removed || ec) {
        if (error) *error = "Could not delete config JSON.";
        return false;
    }
    return true;
}

bool Devils_Den_Config_Store::LoadAppearance(Devils_Den_Config& config, std::string* error)
{
    std::string text;
    if (!ReadFile(SettingsPath(), text)) {
        if (error) *error = "Settings.json does not exist yet.";
        return false;
    }
    (void)ReadInteger(text, "version", config.version);
    ParseAppearance(text, config);
    return true;
}

bool Devils_Den_Config_Store::SaveAppearance(const Devils_Den_Config& config, std::string* error)
{
    std::error_code ec;
    std::filesystem::create_directories(SettingsPath().parent_path(), ec);
    if (ec) {
        if (error) *error = "Could not create Devilz_Den settings directory.";
        return false;
    }

    std::ofstream stream(SettingsPath(), std::ios::binary | std::ios::trunc);
    if (!stream) {
        if (error) *error = "Could not open Settings.json for writing.";
        return false;
    }

    stream << "{\n";
    stream << "  \"version\": 2,\n";
    WriteAppearance(stream, config, false);
    stream << "}\n";
    if (!stream.good()) {
        if (error) *error = "Writing Settings.json failed.";
        return false;
    }
    return true;
}
}
