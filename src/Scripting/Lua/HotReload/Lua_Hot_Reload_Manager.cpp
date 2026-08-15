#include "Lua_Hot_Reload_Manager.hpp"

#include "Scripting/Lua/Fingerprint/Lua_Fingerprint.hpp"
#include "Scripting/Lua/Lua_Script.hpp"
#include "Scripting/Lua/Lua_Script_Manager.hpp"

#include <chrono>
#include <string>
#include <utility>
#include <vector>

namespace Devilz::Scripting::Lua
{
void Lua_Hot_Reload_Manager::SetEnabled(bool enabled) noexcept
{
    m_enabled = enabled;
    m_status = enabled
        ? "Watching for Lua script changes"
        : "Lua hot reload disabled";
    if (enabled)
        m_nextScan = {};
}

bool Lua_Hot_Reload_Manager::Enabled() const noexcept
{
    return m_enabled;
}

void Lua_Hot_Reload_Manager::SetInterval(std::chrono::milliseconds interval) noexcept
{
    if (interval < std::chrono::milliseconds{50})
        interval = std::chrono::milliseconds{50};
    m_interval = interval;
    m_nextScan = {};
}

std::chrono::milliseconds Lua_Hot_Reload_Manager::Interval() const noexcept
{
    return m_interval;
}

void Lua_Hot_Reload_Manager::Tick(
    Lua_Script_Manager& scripts,
    const Lua_Fingerprint_Manager& fingerprints)
{
    if (!m_enabled)
        return;

    const auto now = std::chrono::steady_clock::now();
    if (m_nextScan != std::chrono::steady_clock::time_point{} && now < m_nextScan)
        return;

    m_nextScan = now + m_interval;
    (void)ScanNow(scripts, fingerprints);
}

std::size_t Lua_Hot_Reload_Manager::ScanNow(
    Lua_Script_Manager& scripts,
    const Lua_Fingerprint_Manager& fingerprints)
{
    if (scripts.Scripts().empty()) {
        m_status = m_enabled
            ? "Watching for Lua script changes"
            : "Lua hot reload disabled";
        return 0;
    }

    ++m_scans;

    struct Candidate
    {
        Lua_Script::Id id{};
        std::string path;
    };

    bool scanHadError = false;
    std::vector<Candidate> changed;
    changed.reserve(scripts.Scripts().size());

    for (const auto& script : scripts.Scripts()) {
        Lua_Script_Fingerprint current;
        std::string error;
        if (!fingerprints.FingerprintScript(script->Path(), current, &error)) {
            ++m_failures;
            scanHadError = true;
            m_status = error.empty()
                ? "Lua hot reload could not fingerprint a script"
                : std::move(error);
            continue;
        }

        if (current.value == script->Fingerprint())
            continue;

        changed.push_back({script->GetId(), script->Path().string()});
    }

    std::size_t reloaded = 0;
    for (const auto& candidate : changed) {
        if (scripts.ReloadScript(candidate.id)) {
            ++m_reloads;
            ++reloaded;
            m_status = "Reloaded Lua script: " + candidate.path;
        } else {
            ++m_failures;
            scanHadError = true;
            m_status = "Lua hot reload failed: " + candidate.path;
        }
    }

    if (changed.empty() && !scanHadError)
        m_status = "Watching for Lua script changes";

    return reloaded;
}

Lua_Hot_Reload_Snapshot Lua_Hot_Reload_Manager::Snapshot() const
{
    return {
        m_enabled,
        m_scans,
        m_reloads,
        m_failures,
        m_status};
}

void Lua_Hot_Reload_Manager::Reset() noexcept
{
    m_enabled = true;
    m_interval = std::chrono::milliseconds{250};
    m_nextScan = {};
    m_scans = 0;
    m_reloads = 0;
    m_failures = 0;
    m_status = "Watching for Lua script changes";
}
}
