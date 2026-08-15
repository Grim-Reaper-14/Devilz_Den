#include "Lua_Feature_Manager.hpp"

#include <algorithm>
#include <utility>

namespace Devilz::Scripting::Lua
{
bool Lua_Feature_Manager::Register(
    Lua_Feature_Owner owner,
    std::string name,
    bool defaultEnabled)
{
    if (name.empty() || Available(owner, name))
        return false;

    m_entries.push_back({owner, std::move(name), defaultEnabled, defaultEnabled});
    return true;
}

bool Lua_Feature_Manager::Unregister(Lua_Feature_Owner owner, std::string_view name) noexcept
{
    const auto it = std::find_if(
        m_entries.begin(),
        m_entries.end(),
        [owner, name](const Lua_Feature_Entry& entry) {
            return entry.owner == owner && entry.name == name;
        });
    if (it == m_entries.end())
        return false;

    m_entries.erase(it);
    return true;
}

bool Lua_Feature_Manager::Available(Lua_Feature_Owner owner, std::string_view name) const noexcept
{
    return Find(owner, name) != nullptr;
}

bool Lua_Feature_Manager::Enabled(Lua_Feature_Owner owner, std::string_view name) const noexcept
{
    const auto* entry = Find(owner, name);
    return entry && entry->enabled;
}

bool Lua_Feature_Manager::Set(
    Lua_Feature_Owner owner,
    std::string_view name,
    bool enabled)
{
    auto* entry = Find(owner, name);
    if (!entry)
        return false;

    entry->enabled = enabled;
    return true;
}

bool Lua_Feature_Manager::Toggle(
    Lua_Feature_Owner owner,
    std::string_view name,
    bool* enabled)
{
    auto* entry = Find(owner, name);
    if (!entry)
        return false;

    entry->enabled = !entry->enabled;
    if (enabled)
        *enabled = entry->enabled;
    return true;
}

bool Lua_Feature_Manager::Reset(Lua_Feature_Owner owner, std::string_view name)
{
    auto* entry = Find(owner, name);
    if (!entry)
        return false;

    entry->enabled = entry->defaultEnabled;
    return true;
}

std::size_t Lua_Feature_Manager::RemoveByOwner(Lua_Feature_Owner owner) noexcept
{
    const auto oldSize = m_entries.size();
    std::erase_if(m_entries, [owner](const Lua_Feature_Entry& entry) {
        return entry.owner == owner;
    });
    return oldSize - m_entries.size();
}

void Lua_Feature_Manager::Clear() noexcept
{
    m_entries.clear();
}

std::size_t Lua_Feature_Manager::Count() const noexcept
{
    return m_entries.size();
}

std::size_t Lua_Feature_Manager::CountByOwner(Lua_Feature_Owner owner) const noexcept
{
    return static_cast<std::size_t>(std::count_if(
        m_entries.begin(),
        m_entries.end(),
        [owner](const Lua_Feature_Entry& entry) { return entry.owner == owner; }));
}

Lua_Feature_Entry* Lua_Feature_Manager::Find(
    Lua_Feature_Owner owner,
    std::string_view name) noexcept
{
    const auto it = std::find_if(
        m_entries.begin(),
        m_entries.end(),
        [owner, name](const Lua_Feature_Entry& entry) {
            return entry.owner == owner && entry.name == name;
        });
    return it == m_entries.end() ? nullptr : &*it;
}

const Lua_Feature_Entry* Lua_Feature_Manager::Find(
    Lua_Feature_Owner owner,
    std::string_view name) const noexcept
{
    const auto it = std::find_if(
        m_entries.begin(),
        m_entries.end(),
        [owner, name](const Lua_Feature_Entry& entry) {
            return entry.owner == owner && entry.name == name;
        });
    return it == m_entries.end() ? nullptr : &*it;
}
}
