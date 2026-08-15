#pragma once

#include "Backend/Logging/LogRecord.hpp"

#include <functional>
#include <string>

namespace Devilz::Scripting::Lua
{
class Lua_Commands;
class Lua_Event_Manager;
class Lua_Feature_Manager;
class Lua_Fingerprint_Manager;
class Lua_Setting_Manager;
class Lua_UI_Manager;

using Lua_Log_Callback = std::function<void(
    Backend::LogLevel,
    std::string,
    std::string)>;

struct Lua_Binding_Context
{
    Lua_Commands* commands{};
    Lua_Event_Manager* events{};
    Lua_Fingerprint_Manager* fingerprints{};
    Lua_Setting_Manager* settings{};
    Lua_Feature_Manager* features{};
    Lua_UI_Manager* ui{};
    Lua_Log_Callback logger;
};
}
