#pragma once

#include <memory>

namespace Devilz::Scripting::Lua
{
class Lua_Binding_Context;
class Lua_Binding_Library;
class Lua_Engine;

namespace Bindings::Logger
{
bool RegisterLogger(Lua_Engine& engine, const Lua_Binding_Context& context);
std::unique_ptr<Lua_Binding_Library> CreateLoggerLibrary();
}
}
