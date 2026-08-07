#pragma once

#include "Module_Manager.hpp"
#include "Pattern_Cache.hpp"
#include "Pattern_Scanner.hpp"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace Devilz::Backend
{
enum class Pattern_Address_Transform : std::uint8_t
{
    Match,
    AddOffset,
    ResolveRelative32
};

struct Pattern_Request
{
    std::string name;
    std::string signature;
    std::optional<std::string> section;
    Module_Section_Access access = Module_Section_Access::ExecutableReadable;
    Pattern_Address_Transform transform = Pattern_Address_Transform::Match;
    std::ptrdiff_t offset = 0;
    std::ptrdiff_t relativeDisplacementOffset = 0;
    std::size_t relativeInstructionSize = 0;
    std::function<bool(Pointer)> validator;
};

struct Pattern_Resolution
{
    std::string name;
    Pointer address;
    bool cacheHit = false;
    Pattern_Scan_Stats statistics{};
};

class Pattern_Batch_Scanner final
{
public:
    explicit Pattern_Batch_Scanner(Pattern_Cache* cache = nullptr) : m_cache(cache) {}

    [[nodiscard]] Result<std::vector<Pattern_Resolution>> Resolve(
        const Module_Info& module,
        const std::vector<Pattern_Request>& requests);

private:
    [[nodiscard]] Result<Pattern_Resolution> ResolveOne(
        const Module_Info& module,
        const Pattern_Request& request);

    [[nodiscard]] Result<Pointer> ApplyTransform(Pointer match, const Pattern_Request& request) const;
    [[nodiscard]] static std::uint64_t FingerprintValue(const Module_Fingerprint& fingerprint) noexcept;

    Pattern_Cache* m_cache = nullptr;
    Pattern_Scanner m_scanner;
};
}
