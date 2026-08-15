#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace Devilz::Scripting::Lua
{
using Lua_Setting_Owner = std::uint64_t;
using Lua_Setting_Value = std::variant<bool, std::int64_t, double, std::string>;

enum class Lua_Setting_Type : std::uint8_t
{
    Unknown,
    Boolean,
    Integer,
    Number,
    String
};

struct Lua_Setting_Entry
{
    Lua_Setting_Owner owner{};
    std::string name;
    Lua_Setting_Value defaultValue;
    Lua_Setting_Value value;
};

class Lua_Setting_Manager final
{
public:
    bool Register(Lua_Setting_Owner owner, std::string name, Lua_Setting_Value defaultValue);
    bool Unregister(Lua_Setting_Owner owner, std::string_view name) noexcept;
    [[nodiscard]] bool Exists(Lua_Setting_Owner owner, std::string_view name) const noexcept;
    [[nodiscard]] const Lua_Setting_Value* Get(Lua_Setting_Owner owner, std::string_view name) const noexcept;
    bool Set(Lua_Setting_Owner owner, std::string_view name, Lua_Setting_Value value);
    bool Reset(Lua_Setting_Owner owner, std::string_view name);

    std::size_t RemoveByOwner(Lua_Setting_Owner owner) noexcept;
    void Clear() noexcept;

    [[nodiscard]] std::size_t Count() const noexcept;
    [[nodiscard]] std::size_t CountByOwner(Lua_Setting_Owner owner) const noexcept;
    [[nodiscard]] Lua_Setting_Type Type(Lua_Setting_Owner owner, std::string_view name) const noexcept;
    [[nodiscard]] static Lua_Setting_Type TypeOf(const Lua_Setting_Value& value) noexcept;
    [[nodiscard]] static std::string_view TypeName(Lua_Setting_Type type) noexcept;

private:
    [[nodiscard]] Lua_Setting_Entry* Find(Lua_Setting_Owner owner, std::string_view name) noexcept;
    [[nodiscard]] const Lua_Setting_Entry* Find(Lua_Setting_Owner owner, std::string_view name) const noexcept;

    std::vector<Lua_Setting_Entry> m_entries;
};
}
