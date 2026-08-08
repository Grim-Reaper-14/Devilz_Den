#pragma once

#include "Backend/Error/Result.hpp"
#include "Backend/Process/Process_Memory_Reader.hpp"

#include <cstddef>
#include <cstdint>

namespace Devilz::Integrations::GTA5_Enhanced
{
enum class GTA_Address_Resolve_Mode : std::uint8_t
{
    Direct,
    AddOffset,
    RipRelative32
};

struct GTA_Address_Resolve_Rule
{
    GTA_Address_Resolve_Mode mode = GTA_Address_Resolve_Mode::Direct;
    std::ptrdiff_t offset = 0;
    std::ptrdiff_t displacementOffset = 0;
    std::size_t instructionSize = 0;
};

class GTA_Address_Resolver final
{
public:
    [[nodiscard]] static Devilz::Backend::Result<std::uintptr_t> Resolve(
        const Devilz::Backend::Process_Memory_Reader& reader,
        std::uintptr_t matchAddress,
        const GTA_Address_Resolve_Rule& rule);
};
}
