#include "Native_Cache.hpp"

#include <mutex>

namespace Devilz::Integrations::GTA5_Enhanced
{
void Native_Cache::Store(Native_Registration registration)
{
    std::unique_lock lock(m_mutex);
    m_entries.insert_or_assign(registration.hash, std::move(registration));
}

std::optional<Native_Registration> Native_Cache::Find(Native_Hash hash) const
{
    std::shared_lock lock(m_mutex);
    const auto it = m_entries.find(hash);
    return it == m_entries.end() ? std::nullopt : std::optional<Native_Registration>(it->second);
}

bool Native_Cache::Contains(Native_Hash hash) const
{
    std::shared_lock lock(m_mutex);
    return m_entries.contains(hash);
}

void Native_Cache::Invalidate(Native_Hash hash)
{
    std::unique_lock lock(m_mutex);
    m_entries.erase(hash);
}

void Native_Cache::Clear()
{
    std::unique_lock lock(m_mutex);
    m_entries.clear();
}

std::size_t Native_Cache::Size() const noexcept
{
    std::shared_lock lock(m_mutex);
    return m_entries.size();
}

std::vector<Native_Registration> Native_Cache::Snapshot() const
{
    std::shared_lock lock(m_mutex);
    std::vector<Native_Registration> result;
    result.reserve(m_entries.size());
    for (const auto& [hash, entry] : m_entries) { (void)hash; result.push_back(entry); }
    return result;
}
}
