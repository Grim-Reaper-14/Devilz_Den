#pragma once

#include "GTA_Native_Types.hpp"

#include <cstdint>
#include <optional>
#include <string_view>

namespace Devilz::Integrations::GTA5_Enhanced
{
enum class GTA_Native_Id : std::uint8_t
{
    GetGameTimer,
    GetHashKey
};

struct GTA_Native_Definition
{
    GTA_Native_Id id{};
    std::string_view name;
    GTA_Native_Hash originalHash = 0;
    GTA_Native_Hash enhancedHash = 0;
};

class GTA_Native_Registry final
{
public:
    static constexpr std::uint64_t SupportedFingerprint = 0x6A4F97F605B81000ULL;

    [[nodiscard]] static constexpr std::optional<GTA_Native_Definition> Find(
        std::uint64_t fingerprint,
        GTA_Native_Id id) noexcept
    {
        if (fingerprint != SupportedFingerprint)
            return std::nullopt;

        switch (id) {
        case GTA_Native_Id::GetGameTimer:
            return GTA_Native_Definition{
                id,
                "GET_GAME_TIMER",
                0x9CD27B0045628463ULL,
                0x1DD05E817C89C737ULL
            };
        case GTA_Native_Id::GetHashKey:
            return GTA_Native_Definition{
                id,
                "GET_HASH_KEY",
                0xD24D37CC275948CCULL,
                0x70E57E9927B6BA58ULL
            };
        default:
            return std::nullopt;
        }
    }
};
}
