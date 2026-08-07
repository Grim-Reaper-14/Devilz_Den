#pragma once

#include "Pointer.hpp"

#include <chrono>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Devilz::Backend
{
struct Pattern_Cache_Key
{
    std::string module;
    std::string name;
    std::uint64_t moduleFingerprint = 0;

    [[nodiscard]] std::string StableKey() const
    {
        return module + "::" + name;
    }
};

struct Pattern_Cache_Entry
{
    Pattern_Cache_Key key;
    Pointer address;
    std::uintptr_t moduleBase = 0;
    std::size_t relativeOffset = 0;
    std::chrono::steady_clock::time_point resolvedAt{};
    std::uint64_t hits = 0;
};

class Pattern_Cache
{
public:
    void Store(Pattern_Cache_Key key, Pointer address, std::uintptr_t moduleBase);

    [[nodiscard]] std::optional<Pointer> Find(const Pattern_Cache_Key& key);
    [[nodiscard]] bool Contains(const Pattern_Cache_Key& key) const;

    void Invalidate(std::string_view module, std::string_view name);
    void InvalidateModule(std::string_view module);
    void InvalidateFingerprint(std::string_view module, std::uint64_t validFingerprint);
    void Clear();

    [[nodiscard]] std::size_t Size() const noexcept;
    [[nodiscard]] std::vector<Pattern_Cache_Entry> Snapshot() const;

private:
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, Pattern_Cache_Entry> m_entries;
};
}
