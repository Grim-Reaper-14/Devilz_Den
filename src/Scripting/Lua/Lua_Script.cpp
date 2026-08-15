#include "Lua_Script.hpp"

#include "Fingerprint/Lua_Fingerprint.hpp"
#include "Lua_Engine.hpp"
#include "Lua_Scheduler.hpp"

#include <sol/sol.hpp>

#include <utility>

namespace Devilz::Scripting::Lua
{
Lua_Script::Lua_Script(Id id, std::filesystem::path path)
    : m_id(id), m_path(std::move(path))
{
}

bool Lua_Script::Load(Lua_Engine& engine, const Lua_Fingerprint_Manager& fingerprints)
{
    m_state = Lua_Script_State::Loading;
    m_lastError.clear();
    m_engineId = engine.GetId();
    m_fingerprint = {};

    if (!engine.Ready()) {
        MarkError(std::string{engine.Status()});
        return false;
    }

    std::string fingerprintError;
    if (!fingerprints.FingerprintScript(m_path, m_fingerprint, &fingerprintError)) {
        MarkError(std::move(fingerprintError));
        return false;
    }

    auto& state = engine.State();
    const sol::object devilzObject = state["devilz"];
    sol::table devilz = devilzObject.is<sol::table>()
        ? devilzObject.as<sol::table>()
        : state.create_named_table("devilz");

    auto script = state.create_table();
    script["id"] = m_id;
    script["path"] = m_path.string();
    script["fingerprint"] = Lua_Fingerprint_Manager::ToHex(m_fingerprint.value);
    script["content_hash"] = Lua_Fingerprint_Manager::ToHex(m_fingerprint.contentHash);
    script["runtime_fingerprint"] = Lua_Fingerprint_Manager::ToHex(m_fingerprint.runtimeFingerprint);
    devilz["script"] = script;

    auto result = state.safe_script_file(m_path.string(), sol::script_pass_on_error);
    if (!result.valid()) {
        const sol::error error = result;
        MarkError(error.what());
        return false;
    }

    m_state = Lua_Script_State::Running;
    return true;
}

void Lua_Script::Unload() noexcept
{
    m_state = Lua_Script_State::Unloaded;
    m_engineId = 0;
    m_fingerprint = {};
}

void Lua_Script::DetachEngine() noexcept
{
    m_engineId = 0;
}

void Lua_Script::Tick(Lua_Engine& engine)
{
    if (m_state != Lua_Script_State::Running)
        return;

    if (engine.GetId() != m_engineId || !engine.Ready()) {
        MarkError("Lua script engine is unavailable");
        return;
    }

    auto result = Lua_Scheduler::Tick(engine);
    if (!result.succeeded)
        MarkError(std::move(result.message));
}

void Lua_Script::MarkError(std::string message)
{
    m_lastError = std::move(message);
    m_state = Lua_Script_State::Error;
}

Lua_Script::Id Lua_Script::GetId() const noexcept
{
    return m_id;
}

const std::filesystem::path& Lua_Script::Path() const noexcept
{
    return m_path;
}

Lua_Script_State Lua_Script::State() const noexcept
{
    return m_state;
}

const std::string& Lua_Script::LastError() const noexcept
{
    return m_lastError;
}

std::uint64_t Lua_Script::EngineId() const noexcept
{
    return m_engineId;
}

const Lua_Script_Fingerprint& Lua_Script::FingerprintInfo() const noexcept
{
    return m_fingerprint;
}

std::uint64_t Lua_Script::Fingerprint() const noexcept
{
    return m_fingerprint.value;
}

std::uint64_t Lua_Script::ContentHash() const noexcept
{
    return m_fingerprint.contentHash;
}

std::uint64_t Lua_Script::RuntimeFingerprint() const noexcept
{
    return m_fingerprint.runtimeFingerprint;
}
}
