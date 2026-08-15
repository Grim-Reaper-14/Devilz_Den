#pragma once

#include "Bindings/Lua_Binding_Context.hpp"
#include "Events/Lua_Event_Manager.hpp"
#include "Features/Lua_Feature_Manager.hpp"
#include "Fingerprint/Lua_Fingerprint.hpp"
#include "Lua_Binding_Library_Manager.hpp"
#include "Lua_Commands.hpp"
#include "Lua_Engine_Manager.hpp"
#include "Lua_Module_Manager.hpp"
#include "Lua_Script_Manager.hpp"
#include "Settings/Lua_Setting_Manager.hpp"

#include <cstddef>
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

    void ConfigureServices(Lua_Log_Callback logger);
    bool Initialize();
    void Shutdown() noexcept;
    void Tick();

    [[nodiscard]] bool Ready() const noexcept;
    [[nodiscard]] std::string_view Status() const noexcept;
    [[nodiscard]] std::string_view LuaVersion() const noexcept;
    [[nodiscard]] std::string_view Sol2Version() const noexcept;
    [[nodiscard]] Lua_Manager_Self_Test_Result RunSelfTest();
    [[nodiscard]] std::size_t ScheduledTaskCount();

    [[nodiscard]] Lua_Engine* PrimaryEngine() noexcept;
    [[nodiscard]] Lua_Engine_Manager& Engines() noexcept;
    [[nodiscard]] Lua_Script_Manager& Scripts() noexcept;
    [[nodiscard]] Lua_Module_Manager& Modules() noexcept;
    [[nodiscard]] Lua_Binding_Library_Manager& Libraries() noexcept;
    [[nodiscard]] Lua_Commands& Commands() noexcept;
    [[nodiscard]] Lua_Event_Manager& Events() noexcept;
    [[nodiscard]] Lua_Setting_Manager& Settings() noexcept;
    [[nodiscard]] Lua_Feature_Manager& Features() noexcept;
    [[nodiscard]] Lua_Fingerprint_Manager& Fingerprints() noexcept;
    [[nodiscard]] const Lua_Fingerprint_Manager& Fingerprints() const noexcept;

private:
    Lua_Manager() = default;

    Lua_Engine_Manager m_engines;
    Lua_Script_Manager m_scripts;
    Lua_Module_Manager m_modules;
    Lua_Binding_Library_Manager m_libraries;
    Lua_Commands m_commands;
    Lua_Event_Manager m_events;
    Lua_Setting_Manager m_settings;
    Lua_Feature_Manager m_features;
    Lua_Fingerprint_Manager m_fingerprints;
    Lua_Binding_Context m_bindingContext;
    Lua_Engine::Id m_primaryEngineId{};
    bool m_initialized{};
    std::string m_status{"Not initialized"};
};
}
