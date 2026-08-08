#pragma once

#include <cstddef>
#include <cstdint>

namespace Devilz::Integrations::GTA5_Enhanced
{
// Minimal ABI-facing view. Keep this layout isolated until verified against the
// active GTA Enhanced build before direct native execution is enabled.
struct scrNativeCallContext
{
    void* returnValue = nullptr;
    std::uint32_t argumentCount = 0;
    void* arguments = nullptr;
    std::uint32_t dataCount = 0;
    std::uint32_t vectorCount = 0;

    [[nodiscard]] bool Valid() const noexcept
    {
        return arguments != nullptr && returnValue != nullptr;
    }
};
}
