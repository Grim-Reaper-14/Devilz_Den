#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace Devilz::Backend
{
struct Memory_Range
{
    std::uintptr_t begin = 0;
    std::size_t size = 0;

    [[nodiscard]] constexpr std::uintptr_t End() const noexcept { return begin + size; }
    [[nodiscard]] constexpr bool Empty() const noexcept { return begin == 0 || size == 0; }
    [[nodiscard]] constexpr bool Contains(std::uintptr_t address, std::size_t bytes = 1) const noexcept
    {
        return !Empty() && address >= begin && bytes <= size && address <= End() - bytes;
    }

    [[nodiscard]] std::span<const std::byte> Bytes() const noexcept
    {
        return Empty() ? std::span<const std::byte>{}
                       : std::span<const std::byte>{reinterpret_cast<const std::byte*>(begin), size};
    }
};
}
