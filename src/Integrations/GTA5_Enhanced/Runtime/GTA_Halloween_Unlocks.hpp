#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace Devilz::Integrations::GTA5_Enhanced
{
inline constexpr std::string_view GTA_Halloween_Unlock_Source =
    "Enhanced freemode.c collectible packed-stat paths";

inline constexpr std::size_t GTA_Halloween_Jack_O_Lantern_Count = 200;
inline constexpr std::size_t GTA_Halloween_Ghosts_Exposed_2023_Count = 10;
inline constexpr std::size_t GTA_Halloween_Ghosts_Exposed_2024_Count = 10;
inline constexpr std::size_t GTA_Halloween_Ghosts_Exposed_2025_Count = 10;
inline constexpr std::size_t GTA_Halloween_Verified_Unique_Count =
    GTA_Halloween_Jack_O_Lantern_Count +
    GTA_Halloween_Ghosts_Exposed_2023_Count +
    GTA_Halloween_Ghosts_Exposed_2024_Count +
    GTA_Halloween_Ghosts_Exposed_2025_Count;

inline void GTAAppendHalloweenPackedBoolRange(
    std::vector<std::int32_t>& output,
    std::int32_t first,
    std::int32_t last)
{
    if (last < first)
        return;

    output.reserve(
        output.size() +
        static_cast<std::size_t>(last - first + 1));

    for (std::int32_t index = first; index <= last; ++index)
        output.push_back(index);
}

[[nodiscard]] inline std::vector<std::int32_t> GTAHalloweenJackOLanternIndices()
{
    std::vector<std::int32_t> indices;
    indices.reserve(GTA_Halloween_Jack_O_Lantern_Count);

    // Halloween 2022 Jack O' Lantern collectible locations. The first ten
    // packed BOOLs occupy their own range; locations 10-199 continue at 34512.
    GTAAppendHalloweenPackedBoolRange(indices, 34252, 34261);
    GTAAppendHalloweenPackedBoolRange(indices, 34512, 34701);
    return indices;
}

[[nodiscard]] inline std::vector<std::int32_t> GTAHalloweenGhostsExposed2023Indices()
{
    std::vector<std::int32_t> indices;
    indices.reserve(GTA_Halloween_Ghosts_Exposed_2023_Count);
    GTAAppendHalloweenPackedBoolRange(indices, 41316, 41325);
    return indices;
}

[[nodiscard]] inline std::vector<std::int32_t> GTAHalloweenGhostsExposed2024Indices()
{
    std::vector<std::int32_t> indices;
    indices.reserve(GTA_Halloween_Ghosts_Exposed_2024_Count);
    GTAAppendHalloweenPackedBoolRange(indices, 42258, 42267);
    return indices;
}

[[nodiscard]] inline std::vector<std::int32_t> GTAHalloweenGhostsExposed2025Indices()
{
    std::vector<std::int32_t> indices;
    indices.reserve(GTA_Halloween_Ghosts_Exposed_2025_Count);
    GTAAppendHalloweenPackedBoolRange(indices, 54654, 54663);
    return indices;
}

[[nodiscard]] inline std::vector<std::int32_t> GTAAllVerifiedHalloweenUnlockIndices()
{
    std::vector<std::int32_t> indices;
    indices.reserve(GTA_Halloween_Verified_Unique_Count);

    GTAAppendHalloweenPackedBoolRange(indices, 34252, 34261);
    GTAAppendHalloweenPackedBoolRange(indices, 34512, 34701);
    GTAAppendHalloweenPackedBoolRange(indices, 41316, 41325);
    GTAAppendHalloweenPackedBoolRange(indices, 42258, 42267);
    GTAAppendHalloweenPackedBoolRange(indices, 54654, 54663);
    return indices;
}
}
