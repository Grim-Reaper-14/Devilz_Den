#include "Native_Crossmap.hpp"

#include "Backend/Logging/Logger_Manager.hpp"

#include <charconv>
#include <iomanip>
#include <sstream>

namespace Devilz::Integrations::GTA5_Enhanced
{
using namespace Devilz::Backend;

std::size_t Native_Crossmap::KeyHasher::operator()(const Key& key) const noexcept
{
    const auto a = std::hash<std::uint64_t>{}(key.hash);
    const auto b = std::hash<std::uint64_t>{}(key.build);
    return a ^ (b + 0x9e3779b97f4a7c15ull + (a << 6) + (a >> 2));
}

Result<void> Native_Crossmap::Add(Native_Crossmap_Entry entry)
{
    if (entry.canonical.Value() == 0 || entry.runtime.Value() == 0)
        return Result<void>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
            "Crossmap entry contains a null native hash").With("Name", entry.name));

    const Key canonical{entry.canonical.Value(), entry.buildFingerprint};
    const Key runtime{entry.runtime.Value(), entry.buildFingerprint};

    std::unique_lock lock(m_mutex);
    const auto existingCanonical = m_byCanonical.find(canonical);
    if (existingCanonical != m_byCanonical.end() && existingCanonical->second.runtime != entry.runtime) {
        ++m_stats.collisions;
        return Result<void>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
            "Crossmap canonical hash collision").With("Name", entry.name));
    }
    const auto existingRuntime = m_byRuntime.find(runtime);
    if (existingRuntime != m_byRuntime.end() && existingRuntime->second.canonical != entry.canonical) {
        ++m_stats.collisions;
        return Result<void>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
            "Crossmap runtime hash collision").With("Name", entry.name));
    }

    m_byCanonical.insert_or_assign(canonical, entry);
    m_byRuntime.insert_or_assign(runtime, std::move(entry));
    m_stats.entries = m_byCanonical.size();
    return Result<void>::Success();
}

Result<void> Native_Crossmap::AddBatch(std::vector<Native_Crossmap_Entry> entries)
{
    for (auto& entry : entries) {
        auto result = Add(std::move(entry));
        if (!result) return result;
    }
    return Result<void>::Success();
}

std::optional<Native_Hash> Native_Crossmap::ToRuntime(Native_Hash canonical, std::uint64_t buildFingerprint) const
{
    std::unique_lock lock(m_mutex);
    const auto it = m_byCanonical.find({canonical.Value(), buildFingerprint});
    if (it == m_byCanonical.end()) { ++m_stats.misses; return std::nullopt; }
    ++m_stats.translated;
    return it->second.runtime;
}

std::optional<Native_Hash> Native_Crossmap::ToCanonical(Native_Hash runtime, std::uint64_t buildFingerprint) const
{
    std::unique_lock lock(m_mutex);
    const auto it = m_byRuntime.find({runtime.Value(), buildFingerprint});
    if (it == m_byRuntime.end()) { ++m_stats.misses; return std::nullopt; }
    ++m_stats.translated;
    return it->second.canonical;
}

bool Native_Crossmap::ContainsCanonical(Native_Hash canonical, std::uint64_t buildFingerprint) const
{
    std::shared_lock lock(m_mutex);
    return m_byCanonical.contains({canonical.Value(), buildFingerprint});
}

bool Native_Crossmap::ContainsRuntime(Native_Hash runtime, std::uint64_t buildFingerprint) const
{
    std::shared_lock lock(m_mutex);
    return m_byRuntime.contains({runtime.Value(), buildFingerprint});
}

Result<void> Native_Crossmap::SaveText(File_System_Manager& files, const std::filesystem::path& path) const
{
    std::ostringstream out;
    out << "DDXMAP1\n";
    out << std::hex << std::uppercase;
    for (const auto& entry : Snapshot())
        out << entry.buildFingerprint << '\t' << entry.canonical.Value() << '\t'
            << entry.runtime.Value() << '\t' << entry.name << '\n';
    return files.WriteText(path, out.str(), true);
}

Result<void> Native_Crossmap::LoadText(File_System_Manager& files, const std::filesystem::path& path,
                                       std::uint64_t expectedBuildFingerprint)
{
    auto text = files.ReadText(path);
    if (!text) return Result<void>::Failure(text.Failure());

    std::istringstream input(text.Value());
    std::string line;
    if (!std::getline(input, line) || line != "DDXMAP1")
        return Result<void>::Failure(Error(ErrorCode::FileReadFailed, ErrorCategory::Filesystem,
            "Unsupported native crossmap format").With("Path", path.string()));

    std::vector<Native_Crossmap_Entry> parsed;
    while (std::getline(input, line)) {
        const auto a = line.find('\t');
        const auto b = a == std::string::npos ? a : line.find('\t', a + 1);
        const auto c = b == std::string::npos ? b : line.find('\t', b + 1);
        if (a == std::string::npos || b == std::string::npos || c == std::string::npos) continue;

        std::uint64_t build = 0, canonical = 0, runtime = 0;
        const auto parseHex = [](std::string_view value, std::uint64_t& output) {
            return std::from_chars(value.data(), value.data() + value.size(), output, 16).ec == std::errc{};
        };
        if (!parseHex(std::string_view(line).substr(0, a), build) ||
            !parseHex(std::string_view(line).substr(a + 1, b - a - 1), canonical) ||
            !parseHex(std::string_view(line).substr(b + 1, c - b - 1), runtime)) continue;
        if (expectedBuildFingerprint != 0 && build != expectedBuildFingerprint) continue;
        parsed.push_back({Native_Hash(canonical), Native_Hash(runtime), build, line.substr(c + 1)});
    }

    auto result = AddBatch(std::move(parsed));
    if (result)
        Logger_Manager::Instance().Info("Loaded native crossmap entries", "GTA.Native_Crossmap");
    return result;
}

void Native_Crossmap::InvalidateBuild(std::uint64_t buildFingerprint)
{
    std::unique_lock lock(m_mutex);
    std::erase_if(m_byCanonical, [&](const auto& pair) { return pair.first.build == buildFingerprint; });
    std::erase_if(m_byRuntime, [&](const auto& pair) { return pair.first.build == buildFingerprint; });
    m_stats.entries = m_byCanonical.size();
}

void Native_Crossmap::Clear()
{
    std::unique_lock lock(m_mutex);
    m_byCanonical.clear();
    m_byRuntime.clear();
    m_stats = {};
}

std::vector<Native_Crossmap_Entry> Native_Crossmap::Snapshot() const
{
    std::shared_lock lock(m_mutex);
    std::vector<Native_Crossmap_Entry> entries;
    entries.reserve(m_byCanonical.size());
    for (const auto& [key, entry] : m_byCanonical) { (void)key; entries.push_back(entry); }
    return entries;
}

Native_Crossmap_Stats Native_Crossmap::Statistics() const noexcept
{
    std::shared_lock lock(m_mutex);
    auto stats = m_stats;
    stats.entries = m_byCanonical.size();
    return stats;
}
}
