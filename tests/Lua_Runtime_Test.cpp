#include "Scripting/Lua/Lua_Manager.hpp"
#include "Scripting/Lua/Lua_Runtime.hpp"

#include <iostream>

int main()
{
    auto& runtime = Devilz::Scripting::Lua::Lua_Runtime::Instance();
    runtime.Initialize();

    if (!runtime.Ready()) {
        std::cerr << "Lua runtime failed to initialize: " << runtime.Status() << '\n';
        return 1;
    }

    const auto result = runtime.RunSelfTest();
    if (!result.succeeded) {
        std::cerr << "Lua runtime self-test failed: " << result.message << '\n';
        return 1;
    }

    auto& manager = Devilz::Scripting::Lua::Lua_Manager::Instance();
    auto* engine = manager.PrimaryEngine();
    if (!engine) {
        std::cerr << "Lua primary engine was not created\n";
        return 1;
    }

    auto commandScript = engine->State().safe_script(
        "assert(devilz.commands.register('lua_test_command', function() return true end))",
        sol::script_pass_on_error);
    if (!commandScript.valid()) {
        const sol::error error = commandScript;
        std::cerr << "Lua command registration failed: " << error.what() << '\n';
        return 1;
    }

    const auto commandResult = manager.Commands().Execute("lua_test_command");
    if (!commandResult.succeeded) {
        std::cerr << "Lua command execution failed: " << commandResult.message << '\n';
        return 1;
    }

    runtime.Shutdown();
    if (runtime.Ready()) {
        std::cerr << "Lua runtime remained ready after shutdown\n";
        return 1;
    }

    runtime.Initialize();
    if (!runtime.Ready()) {
        std::cerr << "Lua runtime failed to reinitialize: " << runtime.Status() << '\n';
        return 1;
    }

    runtime.Shutdown();
    return 0;
}
