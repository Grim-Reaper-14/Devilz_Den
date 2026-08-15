#pragma once

#include "Backend/Logging/LogRecord.hpp"

#include <functional>
#include <string>

namespace Devilz::Scripting::Lua
{
class Lua_Commands;
class Lua_Event_Manager;
class Lua_Fingerprint_Manager;

using Lua_Log_Callback = std::function<void(
    Backend::LogLevel,
    std::string,
    std::string)>;

struct Lua_Binding_Context
{
    Lua_Commands* commands{};
    Lua_Event_Manager* events{};
    Lua_Fingerprint_Manager* fingerprints{};
    Lua_Log_Callback logger;
};
}
