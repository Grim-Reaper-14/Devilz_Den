#include "Lua_Runtime.hpp"

#include <sol/sol.hpp>

#include <exception>
#include <utility>

namespace Devilz::Scripting::Lua
{
class Lua_Runtime::Impl final
{
public:
    std::unique_ptr<sol::state> state;
    bool initializationAttempted{};
    std::string status{"Not initialized"};
};

Lua_Runtime& Lua_Runtime::Instance()
{
    static Lua_Runtime runtime;
    return runtime;
}

Lua_Runtime::Lua_Runtime()
    : m_impl(std::make_unique<Impl>())
{
}

Lua_Runtime::~Lua_Runtime() = default;

void Lua_Runtime::Initialize()
{
    if (m_impl->initializationAttempted)
        return;

    m_impl->initializationAttempted = true;

    try {
        auto state = std::make_unique<sol::state>();
        state->open_libraries(
            sol::lib::base,
            sol::lib::coroutine,
            sol::lib::math,
            sol::lib::string,
            sol::lib::table,
            sol::lib::utf8);
        (*state)["dofile"] = sol::nil;
        (*state)["loadfile"] = sol::nil;

        auto devilzApi = state->create_named_table("devilz");
        devilzApi["api_version"] = 1;
        devilzApi["runtime"] = "Devilz Den";

        m_impl->state = std::move(state);
        m_impl->status = "Sol2 runtime initialized";
    } catch (const std::exception& error) {
        m_impl->state.reset();
        m_impl->status = error.what();
    } catch (...) {
        m_impl->state.reset();
        m_impl->status = "Unknown Lua initialization error";
    }
}

void Lua_Runtime::Shutdown() noexcept
{
    m_impl->state.reset();
    m_impl->initializationAttempted = false;
    m_impl->status.clear();
}

bool Lua_Runtime::Ready() const noexcept
{
    return m_impl->state != nullptr;
}

std::string_view Lua_Runtime::Status() const noexcept
{
    return m_impl->status.empty() ? std::string_view{"Not initialized"} : m_impl->status;
}

std::string_view Lua_Runtime::LuaVersion() const noexcept
{
    return LUA_VERSION;
}

std::string_view Lua_Runtime::Sol2Version() const noexcept
{
    return SOL_VERSION_STRING;
}

Lua_Self_Test_Result Lua_Runtime::RunSelfTest()
{
    if (!m_impl->state)
        return {false, "Lua runtime is not initialized"};

    auto result = m_impl->state->safe_script(
        "assert(dofile == nil and loadfile == nil)\n"
        "assert(devilz.api_version == 1)\n"
        "return 6 * 7",
        sol::script_pass_on_error);
    if (!result.valid()) {
        const sol::error error = result;
        return {false, error.what()};
    }

    const int value = result.get<int>();
    if (value != 42)
        return {false, "Lua returned an unexpected self-test value"};

    return {true, "Sol2 executed Lua successfully (6 * 7 = 42)"};
}
}
