#pragma once

#include "Memory_Range.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <type_traits>

namespace Devilz::Backend
{
class Pointer
{
public:
    constexpr Pointer() noexcept = default;
    explicit constexpr Pointer(std::uintptr_t address) noexcept : m_address(address) {}
    explicit Pointer(const void* address) noexcept : m_address(reinterpret_cast<std::uintptr_t>(address)) {}

    [[nodiscard]] constexpr std::uintptr_t Address() const noexcept { return m_address; }
    [[nodiscard]] constexpr bool Valid() const noexcept { return m_address != 0; }
    explicit constexpr operator bool() const noexcept { return Valid(); }

    [[nodiscard]] constexpr Pointer Add(std::ptrdiff_t offset) const noexcept
    {
        return Pointer(static_cast<std::uintptr_t>(static_cast<std::intptr_t>(m_address) + offset));
    }

    [[nodiscard]] constexpr Pointer Sub(std::ptrdiff_t offset) const noexcept { return Add(-offset); }
    [[nodiscard]] constexpr std::ptrdiff_t Distance(Pointer other) const noexcept
    {
        return static_cast<std::ptrdiff_t>(m_address - other.m_address);
    }

    [[nodiscard]] constexpr bool InRange(const Memory_Range& range, std::size_t bytes = 1) const noexcept
    {
        return range.Contains(m_address, bytes);
    }

    template <typename T>
    [[nodiscard]] T* As() const noexcept
    {
        static_assert(!std::is_function_v<T>);
        return reinterpret_cast<T*>(m_address);
    }

    template <typename T>
    [[nodiscard]] const T* AsConst() const noexcept
    {
        return reinterpret_cast<const T*>(m_address);
    }

    // Resolves a signed 32-bit displacement stored at this + displacementOffset.
    // instructionSize is the number of bytes from this pointer to the address
    // immediately following the instruction.
    [[nodiscard]] std::optional<Pointer> ResolveRelative32(
        std::ptrdiff_t displacementOffset,
        std::size_t instructionSize,
        const Memory_Range& readableRange) const noexcept;

    // Follows pointer-sized values using the supplied offsets. Every read must
    // remain inside readableRange; this utility never reads arbitrary remote memory.
    [[nodiscard]] std::optional<Pointer> Follow(
        const std::ptrdiff_t* offsets,
        std::size_t count,
        const Memory_Range& readableRange) const noexcept;

private:
    std::uintptr_t m_address = 0;
};
}
