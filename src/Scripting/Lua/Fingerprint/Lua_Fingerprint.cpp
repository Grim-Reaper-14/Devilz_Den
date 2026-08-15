#include "Lua_Fingerprint.hpp"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <utility>

namespace Devilz::Scripting::Lua
{
namespace
{
constexpr std::uint64_t FnvOffset = 14695981039346656037ull;
constexpr std::uint64_t FnvPrime = 1099511628211ull;

void HashBytes(std::uint64_t& hash, const void* data, std::size_t size) noexcept
{
    const auto* bytes = static_cast<const unsigned char*>(data);
    for (std::size_t index = 0; index < size; ++index) {
        hash ^= bytes[index];
        hash *= FnvPrime;
    }
}

void HashString(std::uint64_t& hash, std::string_view value) noexcept
{
    HashBytes(hash, value.data(), value.size());
    const unsigned char separator = 0xff;
    HashBytes(hash, &separator, 1);
}

void HashU32(std::uint64_t& hash, std::uint32_t value) noexcept
{
    for (unsigned shift = 0; shift < 32; shift += 8) {
        const auto byte = static_cast<unsigned char>((value >> shift) & 0xffu);
        HashBytes(hash, &byte, 1);
    }
}

void HashU64(std::uint64_t& hash, std::uint64_t value) noexcept
{
    for (unsigned shift = 0; shift < 64; shift += 8) {
        const auto byte = static_cast<unsigned char>((value >> shift) & 0xffull);
        HashBytes(hash, &byte, 1);
    }
}
}

bool Lua_Fingerprint_Manager::Initialize(
    std::uint32_t apiVersion,
    std::string_view luaVersion,
    std::string_view sol2Version)
{
    Lua_Runtime_Fingerprint next;
    next.apiVersion = apiVersion;
    next.luaVersion = std::string{luaVersion};
    next.sol2Version = std::string{sol2Version};

    std::uint64_t hash = FnvOffset;
    HashString(hash, "Devilz_Den.Lua.Runtime");
    HashU32(hash, apiVersion);
    HashString(hash, luaVersion);
    HashString(hash, sol2Version);
    HashString(hash, "binding.core.v1");
    HashString(hash, "binding.logger.v1");
    HashString(hash, "binding.events.v1");
    HashString(hash, "binding.settings.v1");
    HashString(hash, "binding.features.v1");
    HashString(hash, "scheduler.v1");
    HashString(hash, "hotreload.v1");
    next.value = hash;

    m_runtime = std::move(next);
    return m_runtime.value != 0;
}

void Lua_Fingerprint_Manager::Reset() noexcept
{
    m_runtime = {};
}

const Lua_Runtime_Fingerprint& Lua_Fingerprint_Manager::Runtime() const noexcept
{
    return m_runtime;
}

bool Lua_Fingerprint_Manager::FingerprintScript(
    const std::filesystem::path& path,
    Lua_Script_Fingerprint& fingerprint,
    std::string* error) const
{
    if (m_runtime.value == 0) {
        if (error)
            *error = "Lua runtime fingerprint is not initialized";
        return false;
    }

    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        if (error)
            *error = "Unable to open Lua script for fingerprinting: " + path.string();
        return false;
    }

    std::uint64_t contentHash = FnvOffset;
    char buffer[4096];
    while (stream) {
        stream.read(buffer, sizeof(buffer));
        const auto count = stream.gcount();
        if (count > 0)
            HashBytes(contentHash, buffer, static_cast<std::size_t>(count));
    }

    if (!stream.eof()) {
        if (error)
            *error = "Unable to read Lua script for fingerprinting: " + path.string();
        return false;
    }

    std::uint64_t combined = FnvOffset;
    HashString(combined, "Devilz_Den.Lua.Script");
    HashU64(combined, contentHash);
    HashU64(combined, m_runtime.value);

    fingerprint.contentHash = contentHash;
    fingerprint.runtimeFingerprint = m_runtime.value;
    fingerprint.value = combined;
    if (error)
        error->clear();
    return true;
}

std::string Lua_Fingerprint_Manager::ToHex(std::uint64_t value)
{
    std::ostringstream stream;
    stream << "0x" << std::uppercase << std::hex << std::setw(16) << std::setfill('0') << value;
    return stream.str();
}
}
