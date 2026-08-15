#include "Lua_Bindings.hpp"

#include "Bindings/Core/Lua_Core_Binding.hpp"
#include "Bindings/Lua_Binding_Context.hpp"

namespace Devilz::Scripting::Lua::Lua_Bindings
{
bool RegisterCore(Lua_Engine& engine, Lua_Commands& commands)
{
    Lua_Binding_Context context;
    context.commands = &commands;
    return Bindings::Core::RegisterCore(engine, context);
}

std::unique_ptr<Lua_Binding_Library> CreateCoreLibrary()
{
    return Bindings::Core::CreateCoreLibrary();
}
}
