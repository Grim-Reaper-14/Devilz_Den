#pragma once

#include <chrono>
#include <cstddef>
#include <string>

namespace Devilz::Scripting::Lua
{
class Lua_Fingerprint_Manager;
class Lua_Script_Manager;

struct Lua_Hot_Reload_Snapshot
{
    bool enabled{true};
    std::size_t scans{};
    std::size_t reloads{};
    std::size_t failures{};
    std::string status{"Watching for Lua script changes"};
};

class Lua_Hot_Reload_Manager final
{
public:
    void SetEnabled(bool enabled) noexcept;
    [[nodiscard]] bool Enabled() const noexcept;

    void SetInterval(std::chrono::milliseconds interval) noexcept;
    [[nodiscard]] std::chrono::milliseconds Interval() const noexcept;

    void Tick(Lua_Script_Manager& scripts, const Lua_Fingerprint_Manager& fingerprints);
    [[nodiscard]] std::size_t ScanNow(
        Lua_Script_Manager& scripts,
        const Lua_Fingerprint_Manager& fingerprints);

    [[nodiscard]] Lua_Hot_Reload_Snapshot Snapshot() const;
    void Reset() noexcept;

private:
    bool m_enabled{true};
    std::chrono::milliseconds m_interval{250};
    std::chrono::steady_clock::time_point m_nextScan{};
    std::size_t m_scans{};
    std::size_t m_reloads{};
    std::size_t m_failures{};
    std::string m_status{"Watching for Lua script changes"};
};
}
