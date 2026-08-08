#pragma once

#include "Backend/Memory/Pointer.hpp"

#include <cstddef>
#include <cstdint>

namespace Devilz::Integrations::GTA5_Enhanced
{
struct scrProgram
{
    std::uint32_t hash = 0;
    std::uint32_t nativeCount = 0;
    Devilz::Backend::Pointer nativeEntrypoints;
    Devilz::Backend::Pointer codePages;
    Devilz::Backend::Pointer statics;
    Devilz::Backend::Pointer globals;
    std::uint32_t codeSize = 0;
    std::uint32_t staticCount = 0;
    std::uint32_t globalCount = 0;

    [[nodiscard]] bool HasNativeTable() const noexcept
    {
        return nativeCount > 0 && !nativeEntrypoints.IsNull();
    }

    [[nodiscard]] bool Valid() const noexcept
    {
        return hash != 0 && (codeSize == 0 || !codePages.IsNull());
    }
};
}
