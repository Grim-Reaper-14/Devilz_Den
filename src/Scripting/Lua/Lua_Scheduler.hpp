#pragma once

#include <cstddef>
#include <string>

namespace Devilz::Scripting::Lua
{
class Lua_Engine;

struct Lua_Scheduler_Tick_Result
{
    bool succeeded{};
    std::string message;
};

class Lua_Scheduler final
{
public:
    static bool Install(Lua_Engine& engine);
    [[nodiscard]] static Lua_Scheduler_Tick_Result Tick(Lua_Engine& engine);
    [[nodiscard]] static std::size_t TaskCount(Lua_Engine& engine);
};
}
