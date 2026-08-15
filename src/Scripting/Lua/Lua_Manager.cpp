#include "Lua_Manager.hpp"

#include "Lua_Bindings.hpp"

#include <sol/sol.hpp>

namespace Devilz::Scripting::Lua
{
Lua_Manager& Lua_Manager::Instance()
{
    static Lua_Manager manager;
    return manager;
}

bool Lua_Manager::Initialize()
{
    if (m_initialized)
        return true;

    m_scripts.Configure(&m_engines, &m_libraries, &m_commands);

    if (!m_libraries.RegisterLibrary(Lua_Bindings::CreateCoreLibrary())) {
        m_status = "Failed to register core Lua binding library";
        return false;
    }

    auto& engine = m_engines.CreateEngine(0);
    if (!engine.Ready()) {
        m_status = std::string{engine.Status()};
        m_engines.Clear();
        m_libraries.Clear();
        return false;
    }

    if (!m_libraries.BindAll(engine, m_commands)) {
        m_status = "Failed to bind core Lua library";
        m_engines.Clear();
        m_libraries.Clear();
        return false;
    }

    m_primaryEngineId = engine.GetId();
    m_initialized = true;
    m_status = "Lua manager initialized";
    return true;
}

void Lua_Manager::Shutdown() noexcept
{
    m_scripts.UnloadAll();
    m_commands.Clear();
    m_modules.Clear();
    m_engines.Clear();
    m_libraries.Clear();
    m_primaryEngineId = 0;
    m_initialized = false;
    m_status = "Not initialized";
}

void Lua_Manager::Tick()
{
    if (m_initialized)
        m_scripts.Tick();
}

bool Lua_Manager::Ready() const noexcept
{
    if (!m_initialized || m_primaryEngineId == 0)
        return false;
    const auto* engine = m_engines.FindEngine(m_primaryEngineId);
    return engine && engine->Ready();
}

std::string_view Lua_Manager::Status() const noexcept
{
    return m_status;
}

std::string_view Lua_Manager::LuaVersion() const noexcept
{
    return LUA_VERSION;
}

std::string_view Lua_Manager::Sol2Version() const noexcept
{
    return SOL_VERSION_STRING;
}

Lua_Manager_Self_Test_Result Lua_Manager::RunSelfTest()
{
    auto* engine = PrimaryEngine();
    if (!engine || !engine->Ready())
        return {false, "Lua manager is not initialized"};

    auto result = engine->State().safe_script(
        "assert(dofile == nil and loadfile == nil)\n"
        "assert(devilz.api_version == 1)\n"
        "assert(devilz.engine_id > 0)\n"
        "assert(devilz.owner_script_id == 0)\n"
        "assert(type(devilz.commands.register) == 'function')\n"
        "return 6 * 7",
        sol::script_pass_on_error);

    if (!result.valid()) {
        const sol::error error = result;
        return {false, error.what()};
    }

    const int value = result.get<int>();
    if (value != 42)
        return {false, "Lua returned an unexpected self-test value"};

    return {true, "Lua manager executed Sol2 successfully (6 * 7 = 42)"};
}

Lua_Engine* Lua_Manager::PrimaryEngine() noexcept
{
    return m_primaryEngineId == 0 ? nullptr : m_engines.FindEngine(m_primaryEngineId);
}

Lua_Engine_Manager& Lua_Manager::Engines() noexcept
{
    return m_engines;
}

Lua_Script_Manager& Lua_Manager::Scripts() noexcept
{
    return m_scripts;
}

Lua_Module_Manager& Lua_Manager::Modules() noexcept
{
    return m_modules;
}

Lua_Binding_Library_Manager& Lua_Manager::Libraries() noexcept
{
    return m_libraries;
}

Lua_Commands& Lua_Manager::Commands() noexcept
{
    return m_commands;
}
}
