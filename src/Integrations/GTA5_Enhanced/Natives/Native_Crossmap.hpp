#pragma once

#include "Backend/Error/Result.hpp"
#include "Backend/Filesystem/File_System_Manager.hpp"
#include "Native_Hash.hpp"
#include "../Runtime/Build_Info.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Devilz::Integrations::GTA5_Enhanced
{
struct Native_Crossmap_Entry
{
    Native_Hash canonical{};
    Native_Hash runtime{};
    std::uint64_t buildFingerprint = 0;
    std::string name;
};

struct Native_Crossmap_Stats
{
    std::size_t entries = 0;
    std::size_t translated = 0;
    std::size_t misses = 0;
    std::size_t collisions = 0;
};

class Native_Crossmap final
{
public:
    Devilz::Backend::Result<void> Add(Native_Crossmap_Entry entry);
    Devilz::Backend::Result<void> AddBatch(std::vector<Native_Crossmap_Entry> entries);

    [[nodiscard]] std::optional<Native_Hash> ToRuntime(
        Native_Hash canonical,
        std::uint64_t buildFingerprint) const;

    [[nodiscard]] std::optional<Native_Hash> ToCanonical(
        Native_Hash runtime,
        std::uint64_t buildFingerprint) const;

    [[nodiscard]] bool ContainsCanonical(Native_Hash canonical,
                                         std::uint64_t buildFingerprint) const;
    [[nodiscard]] bool ContainsRuntime(Native_Hash runtime,
                                       std::uint64_t buildFingerprint) const;

    Devilz::Backend::Result<void> LoadText(Devilz::Backend::File_System_Manager& files,
                                            const std::filesystem::path& path,
                                            std::uint64_t expectedBuildFingerprint = 0);
    Devilz::Backend::Result<void> SaveText(Devilz::Backend::File_System_Manager& files,
                                            const std::filesystem::path& path) const;

    void InvalidateBuild(std::uint64_t buildFingerprint);
    void Clear();

    [[nodiscard]] std::vector<Native_Crossmap_Entry> Snapshot() const;
    [[nodiscard]] Native_Crossmap_Stats Statistics() const noexcept;

private:
    struct Key
    {
        std::uint64_t hash = 0;
        std::uint64_t build = 0;
        [[nodiscard]] bool operator==(const Key&) const noexcept = default;
    };

    struct KeyHasher
    {
        std::size_t operator()(const Key& key) const noexcept;
    };

    mutable std::shared_mutex m_mutex;
    std::unordered_map<Key, Native_Crossmap_Entry, KeyHasher> m_byCanonical;
    std::unordered_map<Key, Native_Crossmap_Entry, KeyHasher> m_byRuntime;
    mutable Native_Crossmap_Stats m_stats{};
};
}
