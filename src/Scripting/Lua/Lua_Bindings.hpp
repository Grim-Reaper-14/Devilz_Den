#pragma once

#include <memory>

namespace Devilz::Scripting::Lua
{
class Lua_Binding_Library;
class Lua_Commands;
class Lua_Engine;

namespace Lua_Bindings
{
bool RegisterCore(Lua_Engine& engine, Lua_Commands& commands);
std::unique_ptr<Lua_Binding_Library> CreateCoreLibrary();
}
}
