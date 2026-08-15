#include "Lua_Engine.hpp"

#include <stdexcept>

namespace Devilz::Scripting::Lua
{
Lua_Engine::Lua_Engine(Id id, std::uint64_t ownerScriptId)
    : m_id(id), m_ownerScriptId(ownerScriptId)
{
}

Lua_Engine::~Lua_Engine() = default;

bool Lua_Engine::Initialize()
{
    if (m_state)
        return true;

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

        m_state = std::move(state);
        m_status = "Ready";
        return true;
    } catch (const std::exception& error) {
        m_state.reset();
        m_status = error.what();
    } catch (...) {
        m_state.reset();
        m_status = "Unknown Lua engine initialization error";
    }

    return false;
}

void Lua_Engine::Shutdown() noexcept
{
    m_state.reset();
    m_status = "Not initialized";
}

bool Lua_Engine::Ready() const noexcept
{
    return m_state != nullptr;
}

Lua_Engine::Id Lua_Engine::GetId() const noexcept
{
    return m_id;
}

std::uint64_t Lua_Engine::OwnerScriptId() const noexcept
{
    return m_ownerScriptId;
}

std::string_view Lua_Engine::Status() const noexcept
{
    return m_status;
}

sol::state& Lua_Engine::State()
{
    if (!m_state)
        throw std::runtime_error("Lua engine is not initialized");
    return *m_state;
}
}
