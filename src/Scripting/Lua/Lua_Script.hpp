#pragma once

#include "Fingerprint/Lua_Fingerprint.hpp"

#include <cstdint>
#include <filesystem>
#include <string>

namespace Devilz::Scripting::Lua
{
class Lua_Engine;
class Lua_Fingerprint_Manager;

enum class Lua_Script_State : std::uint8_t
{
    Unloaded,
    Loading,
    Running,
    Paused,
    Error
};

class Lua_Script final
{
public:
    using Id = std::uint64_t;

    Lua_Script(Id id, std::filesystem::path path);

    bool Load(Lua_Engine& engine, const Lua_Fingerprint_Manager& fingerprints);
    void Unload() noexcept;
    void Tick(Lua_Engine& engine);
    void MarkError(std::string message);

    [[nodiscard]] Id GetId() const noexcept;
    [[nodiscard]] const std::filesystem::path& Path() const noexcept;
    [[nodiscard]] Lua_Script_State State() const noexcept;
    [[nodiscard]] const std::string& LastError() const noexcept;
    [[nodiscard]] std::uint64_t EngineId() const noexcept;
    [[nodiscard]] const Lua_Script_Fingerprint& FingerprintInfo() const noexcept;
    [[nodiscard]] std::uint64_t Fingerprint() const noexcept;
    [[nodiscard]] std::uint64_t ContentHash() const noexcept;
    [[nodiscard]] std::uint64_t RuntimeFingerprint() const noexcept;

private:
    Id m_id{};
    std::filesystem::path m_path;
    Lua_Script_State m_state{Lua_Script_State::Unloaded};
    std::string m_lastError;
    std::uint64_t m_engineId{};
    Lua_Script_Fingerprint m_fingerprint;
};
}
