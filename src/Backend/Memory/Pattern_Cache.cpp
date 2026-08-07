#include "Pattern_Cache.hpp"

#include <algorithm>

namespace Devilz::Backend
{
void Pattern_Cache::Store(Pattern_Cache_Key key, Pointer address, std::uintptr_t moduleBase)
{
    Pattern_Cache_Entry entry;
    entry.key = std::move(key);
    entry.address = address;
    entry.moduleBase = moduleBase;
    entry.relativeOffset = address.Address() >= moduleBase ? address.Address() - moduleBase : 0;
    entry.resolvedAt = std::chrono::steady_clock::now();

    std::scoped_lock lock(m_mutex);
    m_entries.insert_or_assign(entry.key.StableKey(), std::move(entry));
}

std::optional<Pointer> Pattern_Cache::Find(const Pattern_Cache_Key& key)
{
    std::scoped_lock lock(m_mutex);
    const auto it = m_entries.find(key.StableKey());
    if (it == m_entries.end() || it->second.key.moduleFingerprint != key.moduleFingerprint)
        return std::nullopt;
    ++it->second.hits;
    return it->second.address;
}

bool Pattern_Cache::Contains(const Pattern_Cache_Key& key) const
{
    std::scoped_lock lock(m_mutex);
    const auto it = m_entries.find(key.StableKey());
    return it != m_entries.end() && it->second.key.moduleFingerprint == key.moduleFingerprint;
}

void Pattern_Cache::Invalidate(std::string_view module, std::string_view name)
{
    std::scoped_lock lock(m_mutex);
    m_entries.erase(std::string(module) + "::" + std::string(name));
}

void Pattern_Cache::InvalidateModule(std::string_view module)
{
    std::scoped_lock lock(m_mutex);
    std::erase_if(m_entries, [&](const auto& pair) { return pair.second.key.module == module; });
}

void Pattern_Cache::InvalidateFingerprint(std::string_view module, std::uint64_t validFingerprint)
{
    std::scoped_lock lock(m_mutex);
    std::erase_if(m_entries, [&](const auto& pair) {
        return pair.second.key.module == module && pair.second.key.moduleFingerprint != validFingerprint;
    });
}

void Pattern_Cache::Clear()
{
    std::scoped_lock lock(m_mutex);
    m_entries.clear();
}

std::size_t Pattern_Cache::Size() const noexcept
{
    std::scoped_lock lock(m_mutex);
    return m_entries.size();
}

std::vector<Pattern_Cache_Entry> Pattern_Cache::Snapshot() const
{
    std::scoped_lock lock(m_mutex);
    std::vector<Pattern_Cache_Entry> result;
    result.reserve(m_entries.size());
    for (const auto& [key, entry] : m_entries) {
        (void)key;
        result.push_back(entry);
    }
    return result;
}
}
