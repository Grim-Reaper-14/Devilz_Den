#include "Lua_Setting_Manager.hpp"

#include <algorithm>
#include <utility>

namespace Devilz::Scripting::Lua
{
bool Lua_Setting_Manager::Register(
    Lua_Setting_Owner owner,
    std::string name,
    Lua_Setting_Value defaultValue)
{
    if (name.empty() || Exists(owner, name))
        return false;

    Lua_Setting_Entry entry;
    entry.owner = owner;
    entry.name = std::move(name);
    entry.defaultValue = defaultValue;
    entry.value = std::move(defaultValue);
    m_entries.push_back(std::move(entry));
    return true;
}

bool Lua_Setting_Manager::Unregister(Lua_Setting_Owner owner, std::string_view name) noexcept
{
    const auto it = std::find_if(
        m_entries.begin(),
        m_entries.end(),
        [owner, name](const Lua_Setting_Entry& entry) {
            return entry.owner == owner && entry.name == name;
        });
    if (it == m_entries.end())
        return false;

    m_entries.erase(it);
    return true;
}

bool Lua_Setting_Manager::Exists(Lua_Setting_Owner owner, std::string_view name) const noexcept
{
    return Find(owner, name) != nullptr;
}

const Lua_Setting_Value* Lua_Setting_Manager::Get(
    Lua_Setting_Owner owner,
    std::string_view name) const noexcept
{
    const auto* entry = Find(owner, name);
    return entry ? &entry->value : nullptr;
}

bool Lua_Setting_Manager::Set(
    Lua_Setting_Owner owner,
    std::string_view name,
    Lua_Setting_Value value)
{
    auto* entry = Find(owner, name);
    if (!entry || entry->value.index() != value.index())
        return false;

    entry->value = std::move(value);
    return true;
}

bool Lua_Setting_Manager::Reset(Lua_Setting_Owner owner, std::string_view name)
{
    auto* entry = Find(owner, name);
    if (!entry)
        return false;

    entry->value = entry->defaultValue;
    return true;
}

std::size_t Lua_Setting_Manager::RemoveByOwner(Lua_Setting_Owner owner) noexcept
{
    const auto oldSize = m_entries.size();
    std::erase_if(m_entries, [owner](const Lua_Setting_Entry& entry) {
        return entry.owner == owner;
    });
    return oldSize - m_entries.size();
}

void Lua_Setting_Manager::Clear() noexcept
{
    m_entries.clear();
}

std::size_t Lua_Setting_Manager::Count() const noexcept
{
    return m_entries.size();
}

std::size_t Lua_Setting_Manager::CountByOwner(Lua_Setting_Owner owner) const noexcept
{
    return static_cast<std::size_t>(std::count_if(
        m_entries.begin(),
        m_entries.end(),
        [owner](const Lua_Setting_Entry& entry) { return entry.owner == owner; }));
}

Lua_Setting_Type Lua_Setting_Manager::Type(
    Lua_Setting_Owner owner,
    std::string_view name) const noexcept
{
    const auto* value = Get(owner, name);
    return value ? TypeOf(*value) : Lua_Setting_Type::Unknown;
}

Lua_Setting_Type Lua_Setting_Manager::TypeOf(const Lua_Setting_Value& value) noexcept
{
    switch (value.index()) {
    case 0: return Lua_Setting_Type::Boolean;
    case 1: return Lua_Setting_Type::Integer;
    case 2: return Lua_Setting_Type::Number;
    case 3: return Lua_Setting_Type::String;
    default: return Lua_Setting_Type::Unknown;
    }
}

std::string_view Lua_Setting_Manager::TypeName(Lua_Setting_Type type) noexcept
{
    switch (type) {
    case Lua_Setting_Type::Boolean: return "boolean";
    case Lua_Setting_Type::Integer: return "integer";
    case Lua_Setting_Type::Number: return "number";
    case Lua_Setting_Type::String: return "string";
    default: return "unknown";
    }
}

Lua_Setting_Entry* Lua_Setting_Manager::Find(
    Lua_Setting_Owner owner,
    std::string_view name) noexcept
{
    const auto it = std::find_if(
        m_entries.begin(),
        m_entries.end(),
        [owner, name](const Lua_Setting_Entry& entry) {
            return entry.owner == owner && entry.name == name;
        });
    return it == m_entries.end() ? nullptr : &*it;
}

const Lua_Setting_Entry* Lua_Setting_Manager::Find(
    Lua_Setting_Owner owner,
    std::string_view name) const noexcept
{
    const auto it = std::find_if(
        m_entries.begin(),
        m_entries.end(),
        [owner, name](const Lua_Setting_Entry& entry) {
            return entry.owner == owner && entry.name == name;
        });
    return it == m_entries.end() ? nullptr : &*it;
}
}
