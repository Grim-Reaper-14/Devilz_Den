#pragma once

#include <memory>

namespace Devilz::Scripting::Lua
{
struct Lua_Binding_Context;
class Lua_Binding_Library;
class Lua_Engine;

namespace Bindings::Core
{
bool RegisterCore(Lua_Engine& engine, const Lua_Binding_Context& context);
std::unique_ptr<Lua_Binding_Library> CreateCoreLibrary();
}
}
