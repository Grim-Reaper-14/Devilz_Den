#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

namespace Devilz::Scripting::Lua
{
inline constexpr std::uint32_t Lua_API_Version = 1;

struct Lua_Runtime_Fingerprint
{
    std::uint64_t value{};
    std::uint32_t apiVersion{};
    std::string luaVersion;
    std::string sol2Version;
};

struct Lua_Script_Fingerprint
{
    std::uint64_t contentHash{};
    std::uint64_t runtimeFingerprint{};
    std::uint64_t value{};
};

class Lua_Fingerprint_Manager final
{
public:
    bool Initialize(
        std::uint32_t apiVersion,
        std::string_view luaVersion,
        std::string_view sol2Version);
    void Reset() noexcept;

    [[nodiscard]] const Lua_Runtime_Fingerprint& Runtime() const noexcept;
    [[nodiscard]] bool FingerprintScript(
        const std::filesystem::path& path,
        Lua_Script_Fingerprint& fingerprint,
        std::string* error = nullptr) const;

    [[nodiscard]] static std::string ToHex(std::uint64_t value);

private:
    Lua_Runtime_Fingerprint m_runtime;
};
}
