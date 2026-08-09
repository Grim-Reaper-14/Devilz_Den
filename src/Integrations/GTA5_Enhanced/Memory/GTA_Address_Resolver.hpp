#pragma once

#include "Backend/Error/Result.hpp"
#include "Backend/Process/Process_Memory_Reader.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace Devilz::Integrations::GTA5_Enhanced
{
enum class GTA_Address_Resolve_Op_Type : std::uint8_t
{
    Add,
    RipRelative32
};

struct GTA_Address_Resolve_Op
{
    GTA_Address_Resolve_Op_Type type{};
    std::ptrdiff_t offset{};
};

using GTA_Address_Resolve_Chain = std::vector<GTA_Address_Resolve_Op>;

class GTA_Address_Resolver final
{
public:
    [[nodiscard]] static Devilz::Backend::Result<std::uintptr_t> Resolve(
        const Devilz::Backend::Process_Memory_Reader& reader,
        std::uintptr_t matchAddress,
        const GTA_Address_Resolve_Chain& chain);
};
}
