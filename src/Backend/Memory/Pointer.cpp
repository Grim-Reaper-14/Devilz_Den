#include "Pointer.hpp"

#include <cstring>

namespace Devilz::Backend
{
std::optional<Pointer> Pointer::ResolveRelative32(std::ptrdiff_t displacementOffset,
                                                  std::size_t instructionSize,
                                                  const Memory_Range& readableRange) const noexcept
{
    const auto displacementAddress = Add(displacementOffset);
    if (!displacementAddress.InRange(readableRange, sizeof(std::int32_t)))
        return std::nullopt;

    std::int32_t displacement = 0;
    std::memcpy(&displacement, displacementAddress.AsConst<void>(), sizeof(displacement));

    const auto target = Add(static_cast<std::ptrdiff_t>(instructionSize) + displacement);
    return target.Valid() ? std::optional<Pointer>{target} : std::nullopt;
}

std::optional<Pointer> Pointer::Follow(const std::ptrdiff_t* offsets,
                                       std::size_t count,
                                       const Memory_Range& readableRange) const noexcept
{
    if (!offsets && count != 0)
        return std::nullopt;

    Pointer current = *this;
    for (std::size_t i = 0; i < count; ++i) {
        current = current.Add(offsets[i]);
        if (!current.InRange(readableRange, sizeof(std::uintptr_t)))
            return std::nullopt;

        std::uintptr_t next = 0;
        std::memcpy(&next, current.AsConst<void>(), sizeof(next));
        current = Pointer(next);
        if (!current.Valid())
            return std::nullopt;
    }
    return current;
}
}
