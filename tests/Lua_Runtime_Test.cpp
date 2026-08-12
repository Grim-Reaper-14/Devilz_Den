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
