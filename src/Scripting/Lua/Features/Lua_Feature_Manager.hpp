#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace Devilz::Scripting::Lua
{
using Lua_Feature_Owner = std::uint64_t;

struct Lua_Feature_Entry
{
    Lua_Feature_Owner owner{};
    std::string name;
    bool defaultEnabled{};
    bool enabled{};
};

class Lua_Feature_Manager final
{
public:
    bool Register(Lua_Feature_Owner owner, std::string name, bool defaultEnabled = false);
    bool Unregister(Lua_Feature_Owner owner, std::string_view name) noexcept;
    [[nodiscard]] bool Available(Lua_Feature_Owner owner, std::string_view name) const noexcept;
    [[nodiscard]] bool Enabled(Lua_Feature_Owner owner, std::string_view name) const noexcept;
    bool Set(Lua_Feature_Owner owner, std::string_view name, bool enabled);
    bool Toggle(Lua_Feature_Owner owner, std::string_view name, bool* enabled = nullptr);
    bool Reset(Lua_Feature_Owner owner, std::string_view name);

    std::size_t RemoveByOwner(Lua_Feature_Owner owner) noexcept;
    void Clear() noexcept;

    [[nodiscard]] std::size_t Count() const noexcept;
    [[nodiscard]] std::size_t CountByOwner(Lua_Feature_Owner owner) const noexcept;

private:
    [[nodiscard]] Lua_Feature_Entry* Find(Lua_Feature_Owner owner, std::string_view name) noexcept;
    [[nodiscard]] const Lua_Feature_Entry* Find(Lua_Feature_Owner owner, std::string_view name) const noexcept;

    std::vector<Lua_Feature_Entry> m_entries;
};
}
