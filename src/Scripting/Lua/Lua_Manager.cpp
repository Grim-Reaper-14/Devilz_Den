#include "Lua_Manager.hpp"

#include "Bindings/Core/Lua_Core_Binding.hpp"
#include "Bindings/Events/Lua_Events_Binding.hpp"
#include "Bindings/Logger/Lua_Logger_Binding.hpp"
#include "Lua_Scheduler.hpp"

#include <sol/sol.hpp>

#include <utility>

namespace Devilz::Scripting::Lua
{
Lua_Manager& Lua_Manager::Instance()
{
    static Lua_Manager manager;
    return manager;
}

void Lua_Manager::ConfigureServices(Lua_Log_Callback logger)
{
    if (m_initialized)
        return;
    m_bindingContext.logger = std::move(logger);
}

bool Lua_Manager::Initialize()
{
    if (m_initialized)
        return true;

    m_bindingContext.commands = &m_commands;
    m_bindingContext.events = &m_events;
    m_scripts.Configure(&m_engines, &m_libraries, m_bindingContext);

    if (!m_libraries.RegisterLibrary(Bindings::Core::CreateCoreLibrary())) {
        m_status = "Failed to register core Lua binding library";
        return false;
    }
    if (!m_libraries.RegisterLibrary(Bindings::Logger::CreateLoggerLibrary())) {
        m_status = "Failed to register logger Lua binding library";
        m_libraries.Clear();
        return false;
    }
    if (!m_libraries.RegisterLibrary(Bindings::Events::CreateEventsLibrary())) {
        m_status = "Failed to register events Lua binding library";
        m_libraries.Clear();
        return false;
    }

    auto& engine = m_engines.CreateEngine(0);
    if (!engine.Ready()) {
        m_status = std::string{engine.Status()};
        m_engines.Clear();
        m_libraries.Clear();
        return false;
    }

    if (!m_libraries.BindAll(engine, m_bindingContext)) {
        m_status = "Failed to bind Lua libraries";
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
    m_events.Clear();
    m_commands.Clear();
    m_modules.Clear();
    m_engines.Clear();
    m_libraries.Clear();
    m_scripts.Configure(nullptr, nullptr, {});
    m_bindingContext = {};
    m_primaryEngineId = 0;
    m_initialized = false;
    m_status = "Not initialized";
}

void Lua_Manager::Tick()
{
    if (!m_initialized)
        return;

    const auto eventResult = m_events.Emit("tick");
    if (!eventResult.succeeded)
        m_status = "Lua tick event error: " + eventResult.message;

    if (auto* primary = PrimaryEngine()) {
        auto result = Lua_Scheduler::Tick(*primary);
        if (!result.succeeded)
            m_status = "Primary Lua scheduler error: " + result.message;
    }

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
        "assert(type(devilz.create_thread) == 'function')\n"
        "assert(type(devilz.yield) == 'function')\n"
        "assert(type(devilz.log.info) == 'function')\n"
        "assert(type(devilz.log.available) == 'boolean')\n"
        "assert(type(devilz.events.on) == 'function')\n"
        "assert(type(devilz.events.off) == 'function')\n"
        "devilz.__self_test_counter = 0\n"
        "devilz.__self_test_event_counter = 0\n"
        "devilz.__self_test_event_id = devilz.events.on('__devilz_self_test', function()\n"
        "    devilz.__self_test_event_counter = devilz.__self_test_event_counter + 1\n"
        "end)\n"
        "assert(devilz.__self_test_event_id > 0)\n"
        "if devilz.log.available then\n"
        "    assert(devilz.log.info('Lua self-test', 'SelfTest'))\n"
        "end\n"
        "devilz.create_thread(function()\n"
        "    devilz.__self_test_counter = devilz.__self_test_counter + 1\n"
        "    devilz.yield(0)\n"
        "    devilz.__self_test_counter = devilz.__self_test_counter + 1\n"
        "end)\n"
        "return 6 * 7",
        sol::script_pass_on_error);

    if (!result.valid()) {
        const sol::error error = result;
        return {false, error.what()};
    }

    const int value = result.get<int>();
    if (value != 42)
        return {false, "Lua returned an unexpected self-test value"};

    const auto eventResult = m_events.Emit("__devilz_self_test");
    if (!eventResult.succeeded || eventResult.dispatched != 1)
        return {false, eventResult.message.empty() ? "Lua event self-test did not dispatch" : eventResult.message};

    auto firstTick = Lua_Scheduler::Tick(*engine);
    if (!firstTick.succeeded)
        return {false, firstTick.message};

    auto secondTick = Lua_Scheduler::Tick(*engine);
    if (!secondTick.succeeded)
        return {false, secondTick.message};

    const sol::object devilzObject = engine->State()["devilz"];
    if (!devilzObject.is<sol::table>())
        return {false, "Lua core table disappeared during self-test"};

    auto devilz = devilzObject.as<sol::table>();
    const int counter = devilz.get_or("__self_test_counter", -1);
    const int eventCounter = devilz.get_or("__self_test_event_counter", -1);
    const auto eventId = devilz.get_or(
        "__self_test_event_id",
        Lua_Event_Manager::Subscription_Id{0});

    auto unsubscribeResult = engine->State().safe_script(
        "local removed = devilz.events.off(devilz.__self_test_event_id)\n"
        "devilz.__self_test_counter = nil\n"
        "devilz.__self_test_event_counter = nil\n"
        "devilz.__self_test_event_id = nil\n"
        "return removed",
        sol::script_pass_on_error);

    if (!unsubscribeResult.valid()) {
        const sol::error error = unsubscribeResult;
        return {false, error.what()};
    }

    if (!unsubscribeResult.get<bool>() || eventId == 0)
        return {false, "Lua event subscription could not be removed"};

    if (counter != 2 || Lua_Scheduler::TaskCount(*engine) != 0)
        return {false, "Lua coroutine scheduler did not resume and retire the self-test task"};

    if (eventCounter != 1)
        return {false, "Lua event manager did not execute the self-test callback"};

    return {true, "Sol2, Lua scheduler, logger bindings, and events executed successfully"};
}

std::size_t Lua_Manager::ScheduledTaskCount()
{
    std::size_t count = 0;

    if (auto* primary = PrimaryEngine())
        count += Lua_Scheduler::TaskCount(*primary);

    for (const auto& script : m_scripts.Scripts()) {
        auto* engine = m_engines.FindEngine(script->EngineId());
        if (engine)
            count += Lua_Scheduler::TaskCount(*engine);
    }

    return count;
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

Lua_Event_Manager& Lua_Manager::Events() noexcept
{
    return m_events;
}
}
