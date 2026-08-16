#include "Lua_Manager.hpp"

#include "Bindings/Core/Lua_Core_Binding.hpp"
#include "Bindings/Events/Lua_Events_Binding.hpp"
#include "Bindings/Features/Lua_Features_Binding.hpp"
#include "Bindings/Logger/Lua_Logger_Binding.hpp"
#include "Bindings/Settings/Lua_Settings_Binding.hpp"
#include "Bindings/UI/Lua_UI_Binding.hpp"
#include "Lua_Scheduler.hpp"

#include <sol/sol.hpp>

#include <string>
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

    if (!m_fingerprints.Initialize(Lua_API_Version, LUA_RELEASE, SOL_VERSION_STRING)) {
        m_status = "Failed to initialize Lua runtime fingerprint";
        return false;
    }

    m_bindingContext.commands = &m_commands;
    m_bindingContext.events = &m_events;
    m_bindingContext.fingerprints = &m_fingerprints;
    m_bindingContext.settings = &m_settings;
    m_bindingContext.features = &m_features;
    m_bindingContext.ui = &m_ui;
    m_scripts.Configure(&m_engines, &m_libraries, m_bindingContext);

    if (!m_libraries.RegisterLibrary(Bindings::Core::CreateCoreLibrary())) {
        m_status = "Failed to register core Lua binding library";
        m_fingerprints.Reset();
        return false;
    }
    if (!m_libraries.RegisterLibrary(Bindings::Logger::CreateLoggerLibrary())) {
        m_status = "Failed to register logger Lua binding library";
        m_libraries.Clear();
        m_fingerprints.Reset();
        return false;
    }
    if (!m_libraries.RegisterLibrary(Bindings::Events::CreateEventsLibrary())) {
        m_status = "Failed to register events Lua binding library";
        m_libraries.Clear();
        m_fingerprints.Reset();
        return false;
    }
    if (!m_libraries.RegisterLibrary(Bindings::Settings::CreateSettingsLibrary())) {
        m_status = "Failed to register settings Lua binding library";
        m_libraries.Clear();
        m_fingerprints.Reset();
        return false;
    }
    if (!m_libraries.RegisterLibrary(Bindings::Features::CreateFeaturesLibrary())) {
        m_status = "Failed to register features Lua binding library";
        m_libraries.Clear();
        m_fingerprints.Reset();
        return false;
    }
    if (!m_libraries.RegisterLibrary(Bindings::UI::CreateUILibrary())) {
        m_status = "Failed to register UI Lua binding library";
        m_libraries.Clear();
        m_fingerprints.Reset();
        return false;
    }

    auto& engine = m_engines.CreateEngine(0);
    if (!engine.Ready()) {
        m_status = std::string{engine.Status()};
        m_engines.Clear();
        m_libraries.Clear();
        m_fingerprints.Reset();
        return false;
    }

    if (!m_libraries.BindAll(engine, m_bindingContext)) {
        m_status = "Failed to bind Lua libraries";
        m_engines.Clear();
        m_libraries.Clear();
        m_fingerprints.Reset();
        return false;
    }

    m_primaryEngineId = engine.GetId();
    m_initialized = true;
    m_status = "Lua manager initialized | Fingerprint: " +
        Lua_Fingerprint_Manager::ToHex(m_fingerprints.Runtime().value);
    return true;
}

void Lua_Manager::Shutdown() noexcept
{
    m_scripts.UnloadAll();
    m_ui.Clear();
    m_features.Clear();
    m_settings.Clear();
    m_events.Clear();
    m_commands.Clear();
    m_modules.Clear();
    m_engines.Clear();
    m_libraries.Clear();
    m_scripts.Configure(nullptr, nullptr, {});
    m_bindingContext = {};
    m_fingerprints.Reset();
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
    if (!m_initialized || m_primaryEngineId == 0 || m_fingerprints.Runtime().value == 0)
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
    return LUA_RELEASE;
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
        "assert(type(devilz.fingerprint) == 'string')\n"
        "assert(#devilz.fingerprint == 18)\n"
        "assert(type(devilz.runtime_fingerprint) == 'string')\n"
        "assert(devilz.runtime_fingerprint == devilz.fingerprint)\n"
        "assert(type(devilz.runtime_info) == 'table')\n"
        "assert(devilz.runtime_info.fingerprint == devilz.fingerprint)\n"
        "assert(devilz.runtime_info.api_version == devilz.api_version)\n"
        "assert(type(devilz.commands.register) == 'function')\n"
        "assert(type(devilz.script_vm) == 'table')\n"
        "assert(type(devilz.script_vm.available) == 'function')\n"
        "assert(type(devilz.script_vm.list) == 'function')\n"
        "assert(type(devilz.script_vm.invoke) == 'function')\n"
        "assert(type(devilz.script_vm.status) == 'function')\n"
        "assert(type(devilz.script_vm.metrics) == 'function')\n"
        "assert(type(devilz.script_vm.list()) == 'table')\n"
        "assert(type(devilz.script_vm.metrics()) == 'table')\n"
        "assert(type(devilz.create_thread) == 'function')\n"
        "assert(type(devilz.yield) == 'function')\n"
        "assert(type(devilz.log.info) == 'function')\n"
        "assert(type(devilz.log.available) == 'boolean')\n"
        "assert(type(devilz.events.on) == 'function')\n"
        "assert(type(devilz.events.off) == 'function')\n"
        "assert(type(devilz.settings.register) == 'function')\n"
        "assert(type(devilz.features.register) == 'function')\n"
        "assert(type(devilz.ui.button) == 'function')\n"
        "assert(type(devilz.ui.checkbox) == 'function')\n"
        "assert(type(devilz.ui.slider_float) == 'function')\n"
        "assert(devilz.settings.register('__self_bool', false))\n"
        "assert(devilz.settings.register('__self_int', 7))\n"
        "assert(devilz.settings.register('__self_number', 1.5))\n"
        "assert(devilz.settings.register('__self_string', 'ready'))\n"
        "assert(devilz.settings.type('__self_bool') == 'boolean')\n"
        "assert(devilz.settings.type('__self_int') == 'integer')\n"
        "assert(devilz.settings.type('__self_number') == 'number')\n"
        "assert(devilz.settings.type('__self_string') == 'string')\n"
        "assert(devilz.settings.set('__self_bool', true))\n"
        "assert(devilz.settings.get('__self_bool') == true)\n"
        "assert(not devilz.settings.set('__self_bool', 'wrong-type'))\n"
        "assert(devilz.settings.reset('__self_bool'))\n"
        "assert(devilz.settings.get('__self_bool') == false)\n"
        "assert(devilz.features.register('__self_feature', false))\n"
        "assert(devilz.features.available('__self_feature'))\n"
        "assert(not devilz.features.enabled('__self_feature'))\n"
        "assert(devilz.features.set('__self_feature', true))\n"
        "assert(devilz.features.enabled('__self_feature'))\n"
        "local toggled, enabled = devilz.features.toggle('__self_feature')\n"
        "assert(toggled and enabled == false)\n"
        "assert(devilz.features.reset('__self_feature'))\n"
        "assert(not devilz.features.enabled('__self_feature'))\n"
        "devilz.__self_ui_button_hits = 0\n"
        "devilz.__self_ui_checkbox = false\n"
        "devilz.__self_ui_slider = 0.0\n"
        "devilz.__self_ui_section_id = devilz.ui.section('Self Test UI')\n"
        "devilz.__self_ui_text_id = devilz.ui.text('Snapshot-only renderer')\n"
        "devilz.__self_ui_button_id = devilz.ui.button('Test Button', function()\n"
        "    devilz.__self_ui_button_hits = devilz.__self_ui_button_hits + 1\n"
        "end)\n"
        "devilz.__self_ui_checkbox_id = devilz.ui.checkbox('Test Checkbox', false, function(value)\n"
        "    devilz.__self_ui_checkbox = value\n"
        "end)\n"
        "devilz.__self_ui_slider_id = devilz.ui.slider_float('Test Slider', 0.25, 0.0, 1.0, function(value)\n"
        "    devilz.__self_ui_slider = value\n"
        "end)\n"
        "assert(devilz.__self_ui_section_id > 0 and devilz.__self_ui_text_id > 0)\n"
        "assert(devilz.__self_ui_button_id > 0 and devilz.__self_ui_checkbox_id > 0 and devilz.__self_ui_slider_id > 0)\n"
        "assert(devilz.ui.count() == 5)\n"
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

    const sol::object devilzObject = engine->State()["devilz"];
    if (!devilzObject.is<sol::table>())
        return {false, "Lua core table disappeared during self-test"};

    auto devilz = devilzObject.as<sol::table>();
    const auto uiButtonId = devilz.get_or("__self_ui_button_id", Lua_UI_Manager::Element_Id{0});
    const auto uiCheckboxId = devilz.get_or("__self_ui_checkbox_id", Lua_UI_Manager::Element_Id{0});
    const auto uiSliderId = devilz.get_or("__self_ui_slider_id", Lua_UI_Manager::Element_Id{0});

    const auto buttonResult = m_ui.Activate(uiButtonId);
    const auto checkboxResult = m_ui.SetCheckbox(uiCheckboxId, true);
    const auto sliderResult = m_ui.SetSliderFloat(uiSliderId, 0.75F);
    if (!buttonResult.succeeded || !checkboxResult.succeeded || !sliderResult.succeeded) {
        return {
            false,
            !buttonResult.succeeded ? buttonResult.message :
            (!checkboxResult.succeeded ? checkboxResult.message : sliderResult.message)};
    }

    auto firstTick = Lua_Scheduler::Tick(*engine);
    if (!firstTick.succeeded)
        return {false, firstTick.message};

    auto secondTick = Lua_Scheduler::Tick(*engine);
    if (!secondTick.succeeded)
        return {false, secondTick.message};

    const int counter = devilz.get_or("__self_test_counter", -1);
    const int eventCounter = devilz.get_or("__self_test_event_counter", -1);
    const int uiButtonHits = devilz.get_or("__self_ui_button_hits", -1);
    const bool uiCheckbox = devilz.get_or("__self_ui_checkbox", false);
    const float uiSlider = devilz.get_or("__self_ui_slider", -1.0F);
    const auto eventId = devilz.get_or(
        "__self_test_event_id",
        Lua_Event_Manager::Subscription_Id{0});
    const auto fingerprint = devilz.get_or("fingerprint", std::string{});

    if (fingerprint != Lua_Fingerprint_Manager::ToHex(m_fingerprints.Runtime().value))
        return {false, "Lua runtime fingerprint did not match the C++ fingerprint manager"};

    if (uiButtonHits != 1 || !uiCheckbox || uiSlider < 0.749F || uiSlider > 0.751F)
        return {false, "Lua UI callbacks did not receive marshalled actions"};

    auto cleanupResult = engine->State().safe_script(
        "local event_removed = devilz.events.off(devilz.__self_test_event_id)\n"
        "local feature_removed = devilz.features.unregister('__self_feature')\n"
        "local bool_removed = devilz.settings.unregister('__self_bool')\n"
        "local int_removed = devilz.settings.unregister('__self_int')\n"
        "local number_removed = devilz.settings.unregister('__self_number')\n"
        "local string_removed = devilz.settings.unregister('__self_string')\n"
        "local ui_section_removed = devilz.ui.remove(devilz.__self_ui_section_id)\n"
        "local ui_text_removed = devilz.ui.remove(devilz.__self_ui_text_id)\n"
        "local ui_button_removed = devilz.ui.remove(devilz.__self_ui_button_id)\n"
        "local ui_checkbox_removed = devilz.ui.remove(devilz.__self_ui_checkbox_id)\n"
        "local ui_slider_removed = devilz.ui.remove(devilz.__self_ui_slider_id)\n"
        "devilz.__self_test_counter = nil\n"
        "devilz.__self_test_event_counter = nil\n"
        "devilz.__self_test_event_id = nil\n"
        "devilz.__self_ui_button_hits = nil\n"
        "devilz.__self_ui_checkbox = nil\n"
        "devilz.__self_ui_slider = nil\n"
        "devilz.__self_ui_section_id = nil\n"
        "devilz.__self_ui_text_id = nil\n"
        "devilz.__self_ui_button_id = nil\n"
        "devilz.__self_ui_checkbox_id = nil\n"
        "devilz.__self_ui_slider_id = nil\n"
        "return event_removed and feature_removed and bool_removed and int_removed and number_removed and string_removed and ui_section_removed and ui_text_removed and ui_button_removed and ui_checkbox_removed and ui_slider_removed",
        sol::script_pass_on_error);

    if (!cleanupResult.valid()) {
        const sol::error error = cleanupResult;
        return {false, error.what()};
    }

    if (!cleanupResult.get<bool>() || eventId == 0)
        return {false, "Lua self-test resources could not be removed"};

    if (m_settings.CountByOwner(0) != 0 || m_features.CountByOwner(0) != 0 || m_ui.CountByOwner(0) != 0)
        return {false, "Lua settings/features/UI self-test leaked owner resources"};

    if (counter != 2 || Lua_Scheduler::TaskCount(*engine) != 0)
        return {false, "Lua coroutine scheduler did not resume and retire the self-test task"};

    if (eventCounter != 1)
        return {false, "Lua event manager did not execute the self-test callback"};

    return {
        true,
        "Sol2, scheduler, logger, events, settings, features, UI, and fingerprinting passed | " + fingerprint};
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

Lua_Setting_Manager& Lua_Manager::Settings() noexcept
{
    return m_settings;
}

Lua_Feature_Manager& Lua_Manager::Features() noexcept
{
    return m_features;
}

Lua_UI_Manager& Lua_Manager::UI() noexcept
{
    return m_ui;
}

const Lua_UI_Manager& Lua_Manager::UI() const noexcept
{
    return m_ui;
}

Lua_Fingerprint_Manager& Lua_Manager::Fingerprints() noexcept
{
    return m_fingerprints;
}

const Lua_Fingerprint_Manager& Lua_Manager::Fingerprints() const noexcept
{
    return m_fingerprints;
}
}
