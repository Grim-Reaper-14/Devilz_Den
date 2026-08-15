#pragma once

#include "Lua_Binding_Library_Manager.hpp"
#include "Lua_Commands.hpp"
#include "Lua_Engine_Manager.hpp"
#include "Lua_Module_Manager.hpp"
#include "Lua_Script_Manager.hpp"

#include <string>
#include <string_view>

namespace Devilz::Scripting::Lua
{
struct Lua_Manager_Self_Test_Result
{
    bool succeeded{};
    std::string message;
};

class Lua_Manager final
{
public:
    static Lua_Manager& Instance();

    Lua_Manager(const Lua_Manager&) = delete;
    Lua_Manager& operator=(const Lua_Manager&) = delete;

    bool Initialize();
    void Shutdown() noexcept;
    void Tick();

    [[nodiscard]] bool Ready() const noexcept;
    [[nodiscard]] std::string_view Status() const noexcept;
    [[nodiscard]] std::string_view LuaVersion() const noexcept;
    [[nodiscard]] std::string_view Sol2Version() const noexcept;
    [[nodiscard]] Lua_Manager_Self_Test_Result RunSelfTest();

    [[nodiscard]] Lua_Engine* PrimaryEngine() noexcept;
    [[nodiscard]] Lua_Engine_Manager& Engines() noexcept;
    [[nodiscard]] Lua_Script_Manager& Scripts() noexcept;
    [[nodiscard]] Lua_Module_Manager& Modules() noexcept;
    [[nodiscard]] Lua_Binding_Library_Manager& Libraries() noexcept;
    [[nodiscard]] Lua_Commands& Commands() noexcept;

private:
    Lua_Manager() = default;

    Lua_Engine_Manager m_engines;
    Lua_Script_Manager m_scripts;
    Lua_Module_Manager m_modules;
    Lua_Binding_Library_Manager m_libraries;
    Lua_Commands m_commands;
    Lua_Engine::Id m_primaryEngineId{};
    bool m_initialized{};
    std::string m_status{"Not initialized"};
};
}
