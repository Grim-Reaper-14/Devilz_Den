#pragma once

namespace Devilz::Scripting::Lua
{
class Lua_Config_Manager;
class Lua_Engine;
class Lua_Setting_Manager;

namespace Bindings::Config
{
bool RegisterConfig(
    Lua_Engine& engine,
    Lua_Config_Manager& configs,
    Lua_Setting_Manager& settings);
}
}
