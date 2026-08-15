#include "Lua_Config_Manager.hpp"

#include "Scripting/Lua/Settings/Lua_Setting_Manager.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <system_error>
#include <type_traits>
#include <utility>

namespace Devilz::Scripting::Lua
{
namespace
{
constexpr std::uint64_t FnvOffset = 14695981039346656037ull;
constexpr std::uint64_t FnvPrime = 1099511628211ull;

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

std::size_t ValueStart(std::string_view text, std::string_view key)
{
    const std::string needle = "\"" + std::string{key} + "\"";
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

bool ReadJsonString(std::string_view text, std::string_view key, std::string& output)
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

std::string_view ReadRawValue(std::string_view text)
{
    const auto pos = ValueStart(text, "value");
    if (pos == std::string_view::npos)
        return {};

    const auto end = text.find_last_of('}');
    if (end == std::string_view::npos || end <= pos)
        return {};

    auto value = text.substr(pos, end - pos);
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())))
        value.remove_suffix(1);
    return value;
}

bool ParsePersistedValue(
    std::string_view line,
    Lua_Setting_Type type,
    Lua_Setting_Value& value)
{
    switch (type) {
    case Lua_Setting_Type::Boolean: {
        const auto raw = ReadRawValue(line);
        if (raw == "true") {
            value = true;
            return true;
        }
        if (raw == "false") {
            value = false;
            return true;
        }
        return false;
    }
    case Lua_Setting_Type::Integer: {
        const auto raw = ReadRawValue(line);
        if (raw.empty())
            return false;
        std::int64_t parsed{};
        const auto result = std::from_chars(raw.data(), raw.data() + raw.size(), parsed);
        if (result.ec != std::errc{} || result.ptr != raw.data() + raw.size())
            return false;
        value = parsed;
        return true;
    }
    case Lua_Setting_Type::Number: {
        const auto raw = ReadRawValue(line);
        if (raw.empty())
            return false;
        try {
            std::size_t used = 0;
            const auto parsed = std::stod(std::string{raw}, &used);
            if (used != raw.size())
                return false;
            value = parsed;
            return true;
        } catch (...) {
            return false;
        }
    }
    case Lua_Setting_Type::String: {
        std::string parsed;
        if (!ReadJsonString(line, "value", parsed))
            return false;
        value = std::move(parsed);
        return true;
    }
    default:
        return false;
    }
}

void WriteValue(std::ostream& stream, const Lua_Setting_Value& value)
{
    std::visit(
        [&stream](const auto& current) {
            using Value = std::decay_t<decltype(current)>;
            if constexpr (std::is_same_v<Value, bool>) {
                stream << (current ? "true" : "false");
            } else if constexpr (std::is_same_v<Value, std::string>) {
                stream << '"' << EscapeJson(current) << '"';
            } else if constexpr (std::is_same_v<Value, double>) {
                stream << std::setprecision(17) << current;
            } else {
                stream << current;
            }
        },
        value);
}

std::string SanitizeStem(std::string value)
{
    for (char& c : value) {
        const auto byte = static_cast<unsigned char>(c);
        if (!std::isalnum(byte) && c != '-' && c != '_')
            c = '_';
    }
    if (value.empty())
        value = "script";
    if (value.size() > 40)
        value.resize(40);
    return value;
}
}

bool Lua_Config_Manager::AttachOwner(Owner owner, const std::filesystem::path& scriptPath)
{
    if (owner == 0 || scriptPath.empty())
        return false;

    const auto key = MakeScriptKey(scriptPath);
    if (key.empty())
        return false;

    m_ownerKeys.insert_or_assign(owner, key);
    return true;
}

void Lua_Config_Manager::DetachOwner(Owner owner) noexcept
{
    m_ownerKeys.erase(owner);
}

void Lua_Config_Manager::ClearOwners() noexcept
{
    m_ownerKeys.clear();
}

bool Lua_Config_Manager::Available(Owner owner) const noexcept
{
    return owner != 0 && m_ownerKeys.contains(owner);
}

std::string Lua_Config_Manager::ScriptKey(Owner owner) const
{
    const auto it = m_ownerKeys.find(owner);
    return it == m_ownerKeys.end() ? std::string{} : it->second;
}

Lua_Config_Result Lua_Config_Manager::Save(
    Owner owner,
    std::string_view profile,
    const Lua_Setting_Manager& settings) const
{
    if (!Available(owner))
        return {false, 0, "Lua config is unavailable for this engine"};
    if (!ValidProfile(profile))
        return {false, 0, "Config profile must use only letters, numbers, '-' or '_'"};

    const auto directory = OwnerDirectory(owner);
    std::error_code ec;
    std::filesystem::create_directories(directory, ec);
    if (ec)
        return {false, 0, "Could not create the Lua config directory"};

    std::ofstream stream(ProfilePath(owner, profile), std::ios::binary | std::ios::trunc);
    if (!stream)
        return {false, 0, "Could not open the Lua config profile for writing"};

    const auto entries = settings.SnapshotByOwner(owner);
    stream << "{\n";
    stream << "  \"version\": 1,\n";
    stream << "  \"script_key\": \"" << EscapeJson(ScriptKey(owner)) << "\",\n";
    stream << "  \"profile\": \"" << EscapeJson(profile) << "\",\n";
    stream << "  \"settings\": [\n";

    for (std::size_t index = 0; index < entries.size(); ++index) {
        const auto& entry = entries[index];
        stream << "    {\"name\":\"" << EscapeJson(entry.name)
               << "\",\"type\":\"" << Lua_Setting_Manager::TypeName(
                      Lua_Setting_Manager::TypeOf(entry.value))
               << "\",\"value\":";
        WriteValue(stream, entry.value);
        stream << '}';
        stream << (index + 1U < entries.size() ? ",\n" : "\n");
    }

    stream << "  ]\n";
    stream << "}\n";
    if (!stream.good())
        return {false, 0, "Writing the Lua config profile failed"};

    return {true, entries.size(), "Saved Lua config profile"};
}

Lua_Config_Result Lua_Config_Manager::Load(
    Owner owner,
    std::string_view profile,
    Lua_Setting_Manager& settings) const
{
    if (!Available(owner))
        return {false, 0, "Lua config is unavailable for this engine"};
    if (!ValidProfile(profile))
        return {false, 0, "Config profile must use only letters, numbers, '-' or '_'"};

    std::ifstream stream(ProfilePath(owner, profile), std::ios::binary);
    if (!stream)
        return {false, 0, "Lua config profile does not exist"};

    std::size_t applied = 0;
    std::string line;
    while (std::getline(stream, line)) {
        if (line.find("\"name\"") == std::string::npos)
            continue;

        std::string name;
        std::string typeName;
        if (!ReadJsonString(line, "name", name) || !ReadJsonString(line, "type", typeName))
            return {false, applied, "Lua config profile contains a malformed setting entry"};

        const auto currentType = settings.Type(owner, name);
        if (currentType == Lua_Setting_Type::Unknown ||
            typeName != Lua_Setting_Manager::TypeName(currentType)) {
            continue;
        }

        Lua_Setting_Value value{false};
        if (!ParsePersistedValue(line, currentType, value))
            return {false, applied, "Lua config profile contains an invalid typed value"};

        if (settings.Set(owner, name, std::move(value)))
            ++applied;
    }

    if (!stream.eof())
        return {false, applied, "Reading the Lua config profile failed"};

    return {true, applied, "Loaded Lua config profile"};
}

Lua_Config_Result Lua_Config_Manager::Remove(Owner owner, std::string_view profile) const
{
    if (!Available(owner))
        return {false, 0, "Lua config is unavailable for this engine"};
    if (!ValidProfile(profile))
        return {false, 0, "Config profile must use only letters, numbers, '-' or '_'"};

    std::error_code ec;
    const bool removed = std::filesystem::remove(ProfilePath(owner, profile), ec);
    if (ec || !removed)
        return {false, 0, "Lua config profile could not be removed"};
    return {true, 0, "Removed Lua config profile"};
}

std::vector<std::string> Lua_Config_Manager::List(Owner owner) const
{
    std::vector<std::string> profiles;
    if (!Available(owner))
        return profiles;

    const auto directory = OwnerDirectory(owner);
    std::error_code ec;
    if (!std::filesystem::exists(directory, ec) || ec)
        return profiles;

    for (const auto& entry : std::filesystem::directory_iterator(directory, ec)) {
        if (ec)
            break;
        if (!entry.is_regular_file() || entry.path().extension() != ".json")
            continue;
        profiles.push_back(entry.path().stem().string());
    }

    std::sort(profiles.begin(), profiles.end());
    return profiles;
}

std::filesystem::path Lua_Config_Manager::RootDirectory()
{
    if (const char* localAppData = std::getenv("LOCALAPPDATA"); localAppData && *localAppData)
        return std::filesystem::path{localAppData} / "Devilz_Den" / "Lua" / "Configs";
    return std::filesystem::current_path() / "Devilz_Den" / "Lua" / "Configs";
}

bool Lua_Config_Manager::ValidProfile(std::string_view profile) noexcept
{
    if (profile.empty() || profile.size() > 64)
        return false;

    return std::all_of(profile.begin(), profile.end(), [](char c) {
        const auto byte = static_cast<unsigned char>(c);
        return std::isalnum(byte) || c == '-' || c == '_';
    });
}

std::string Lua_Config_Manager::MakeScriptKey(const std::filesystem::path& path)
{
    std::error_code ec;
    auto normalized = std::filesystem::weakly_canonical(path, ec);
    if (ec) {
        ec.clear();
        normalized = std::filesystem::absolute(path, ec);
        if (ec)
            normalized = path;
        normalized = normalized.lexically_normal();
    }

    std::string identity = normalized.generic_string();
#ifdef _WIN32
    std::transform(identity.begin(), identity.end(), identity.begin(), [](char c) {
        return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    });
#endif

    std::uint64_t hash = FnvOffset;
    for (const char c : identity) {
        hash ^= static_cast<unsigned char>(c);
        hash *= FnvPrime;
    }

    std::ostringstream hex;
    hex << std::uppercase << std::hex << std::setw(16) << std::setfill('0') << hash;
    return SanitizeStem(path.stem().string()) + '_' + hex.str();
}

std::filesystem::path Lua_Config_Manager::OwnerDirectory(Owner owner) const
{
    const auto key = ScriptKey(owner);
    return key.empty() ? std::filesystem::path{} : RootDirectory() / key;
}

std::filesystem::path Lua_Config_Manager::ProfilePath(Owner owner, std::string_view profile) const
{
    if (!ValidProfile(profile))
        return {};
    const auto directory = OwnerDirectory(owner);
    return directory.empty()
        ? std::filesystem::path{}
        : directory / (std::string{profile} + ".json");
}
}
