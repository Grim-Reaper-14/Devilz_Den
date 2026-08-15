#pragma once

#include <string_view>

namespace Devilz::Scripting::Lua
{
class Lua_Binding_Context;
class Lua_Engine;

class Lua_Binding_Library
{
public:
    virtual ~Lua_Binding_Library();

    [[nodiscard]] virtual std::string_view Name() const noexcept = 0;
    virtual bool Register(Lua_Engine& engine, const Lua_Binding_Context& context) = 0;
};
}
