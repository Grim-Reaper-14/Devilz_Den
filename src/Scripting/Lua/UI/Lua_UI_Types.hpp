#pragma once

#include <cstdint>
#include <string>

namespace Devilz::Scripting::Lua
{
using Lua_UI_Element_Id = std::uint64_t;

enum class Lua_UI_Element_Type : std::uint8_t
{
    Section,
    Text,
    Button,
    Checkbox,
    SliderFloat
};

struct Lua_UI_Element_Snapshot
{
    Lua_UI_Element_Id id{};
    std::uint64_t ownerScriptId{};
    Lua_UI_Element_Type type{Lua_UI_Element_Type::Text};
    std::string label;
    bool boolValue{};
    float floatValue{};
    float minValue{};
    float maxValue{};
};

struct Lua_UI_Action_Result
{
    bool succeeded{};
    std::string message;
};
}
