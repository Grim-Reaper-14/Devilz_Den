#pragma once

#include "Integrations/GTA5_Enhanced/Types/Containers/GTA_At_Array.hpp"
#include "Integrations/GTA5_Enhanced/Types/Stats/GTA_Stat_Data.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace Devilz::Integrations::GTA5_Enhanced
{
// Non-owning ABI view of the known CStatsMgr prefix.
//
// The game owns this object. Do not construct or destroy one from Devilz Den.
// Additional, currently unknown CStatsMgr state follows this prefix.
struct alignas(8) GTA_Stats_Manager_View
{
    static constexpr std::size_t KnownPrefixSize = 0x18;

    bool initialized = false;                         // 0x00
    std::array<std::byte, 0x07> unknown01{};         // 0x01
    GTA_At_Array_View<GTA_Stat_Map> stats{};         // 0x08

    [[nodiscard]] GTA_Stat_Data* GetStat(std::uint32_t stat) noexcept
    {
        if (!initialized || !stats.data)
            return nullptr;

        for (std::uint16_t index = 0; index < stats.count; ++index) {
            GTA_Stat_Map& entry = stats.data[index];
            if (entry.hash == stat)
                return entry.data;
        }

        return nullptr;
    }

    [[nodiscard]] const GTA_Stat_Data* GetStat(std::uint32_t stat) const noexcept
    {
        if (!initialized || !stats.data)
            return nullptr;

        for (std::uint16_t index = 0; index < stats.count; ++index) {
            const GTA_Stat_Map& entry = stats.data[index];
            if (entry.hash == stat)
                return entry.data;
        }

        return nullptr;
    }

    [[nodiscard]] bool HasStat(std::uint32_t stat) const noexcept
    {
        return GetStat(stat) != nullptr;
    }

    [[nodiscard]] std::uint16_t StatCount() const noexcept
    {
        return stats.count;
    }
};

static_assert(sizeof(void*) == 0x08);
static_assert(offsetof(GTA_Stats_Manager_View, initialized) == 0x00);
static_assert(offsetof(GTA_Stats_Manager_View, stats) == 0x08);
static_assert(sizeof(GTA_Stats_Manager_View) == GTA_Stats_Manager_View::KnownPrefixSize);
static_assert(alignof(GTA_Stats_Manager_View) == 0x08);
}
