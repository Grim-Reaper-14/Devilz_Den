#pragma once

#include "GTA_Native_Types.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace Devilz::Integrations::GTA5_Enhanced
{
enum class GTA_Native_Id : std::uint8_t
{
    GetGameTimer,
    GetHashKey,
    PlayerPedId,
    SetEntityInvincible,
    PlayerId,
    SetPlayerWantedLevel,
    SetPlayerWantedLevelNow,
    SetMaxWantedLevel,
    SetSuperJumpThisFrame,
    SetPedMaxTimeUnderwater,
    SetPedCanRagdoll,
    ClearPedBloodDamage,
    ClearPedWetness,
    ClearPedEnvDirt,
    ResetPedVisibleDamage,
    SetPedInfiniteAmmo,
    GiveWeaponToPed,
    SetPedAmmo,
    GetVehiclePedIsIn,
    IsWaypointActive,
    GetWaypointBlipEnumId,
    GetClosestBlipInfoId,
    GetBlipCoords,
    RequestCollisionAtCoord,
    GetGroundZFor3DCoord,
    GetWaterHeight,
    GetApproxHeightForPoint,
    SetEntityCoordsNoOffset,
    DisableAllControlActions
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
    static constexpr std::array<GTA_Native_Id, 29> NamedIds{
        GTA_Native_Id::GetGameTimer,
        GTA_Native_Id::GetHashKey,
        GTA_Native_Id::PlayerPedId,
        GTA_Native_Id::SetEntityInvincible,
        GTA_Native_Id::PlayerId,
        GTA_Native_Id::SetPlayerWantedLevel,
        GTA_Native_Id::SetPlayerWantedLevelNow,
        GTA_Native_Id::SetMaxWantedLevel,
        GTA_Native_Id::SetSuperJumpThisFrame,
        GTA_Native_Id::SetPedMaxTimeUnderwater,
        GTA_Native_Id::SetPedCanRagdoll,
        GTA_Native_Id::ClearPedBloodDamage,
        GTA_Native_Id::ClearPedWetness,
        GTA_Native_Id::ClearPedEnvDirt,
        GTA_Native_Id::ResetPedVisibleDamage,
        GTA_Native_Id::SetPedInfiniteAmmo,
        GTA_Native_Id::GiveWeaponToPed,
        GTA_Native_Id::SetPedAmmo,
        GTA_Native_Id::GetVehiclePedIsIn,
        GTA_Native_Id::IsWaypointActive,
        GTA_Native_Id::GetWaypointBlipEnumId,
        GTA_Native_Id::GetClosestBlipInfoId,
        GTA_Native_Id::GetBlipCoords,
        GTA_Native_Id::RequestCollisionAtCoord,
        GTA_Native_Id::GetGroundZFor3DCoord,
        GTA_Native_Id::GetWaterHeight,
        GTA_Native_Id::GetApproxHeightForPoint,
        GTA_Native_Id::SetEntityCoordsNoOffset,
        GTA_Native_Id::DisableAllControlActions
    };

    [[nodiscard]] static constexpr std::optional<GTA_Native_Definition> Find(
        std::uint64_t fingerprint,
        GTA_Native_Id id) noexcept
    {
        if (fingerprint != SupportedFingerprint)
            return std::nullopt;

        switch (id) {
        case GTA_Native_Id::GetGameTimer:
            return GTA_Native_Definition{id, "GET_GAME_TIMER", 0x9CD27B0045628463ULL, 0x1DD05E817C89C737ULL};
        case GTA_Native_Id::GetHashKey:
            return GTA_Native_Definition{id, "GET_HASH_KEY", 0xD24D37CC275948CCULL, 0x70E57E9927B6BA58ULL};
        case GTA_Native_Id::PlayerPedId:
            return GTA_Native_Definition{id, "PLAYER_PED_ID", 0xD80958FC74E988A6ULL, 0x4A8C381C258A124DULL};
        case GTA_Native_Id::SetEntityInvincible:
            return GTA_Native_Definition{id, "SET_ENTITY_INVINCIBLE", 0x3882114BDE571AD4ULL, 0x935364B4448CD584ULL};
        case GTA_Native_Id::PlayerId:
            return GTA_Native_Definition{id, "PLAYER_ID", 0x4F8644AF03D0E0D6ULL, 0x259BE71D8A81D4FAULL};
        case GTA_Native_Id::SetPlayerWantedLevel:
            return GTA_Native_Definition{id, "SET_PLAYER_WANTED_LEVEL", 0x39FF19C64EF7DA5BULL, 0xE20A252886E4FE1DULL};
        case GTA_Native_Id::SetPlayerWantedLevelNow:
            return GTA_Native_Definition{id, "SET_PLAYER_WANTED_LEVEL_NOW", 0xE0A7D1E497FFCD6FULL, 0x42C9A22D6724F283ULL};
        case GTA_Native_Id::SetMaxWantedLevel:
            return GTA_Native_Definition{id, "SET_MAX_WANTED_LEVEL", 0xAA5F02DB48D704B9ULL, 0xDAE61414743C8D1DULL};
        case GTA_Native_Id::SetSuperJumpThisFrame:
            return GTA_Native_Definition{id, "SET_SUPER_JUMP_THIS_FRAME", 0x57FFF03E423A4C0BULL, 0x353BF8D85390AA39ULL};
        case GTA_Native_Id::SetPedMaxTimeUnderwater:
            return GTA_Native_Definition{id, "SET_PED_MAX_TIME_UNDERWATER", 0x6BA428C528D9E522ULL, 0x0ACCC8916441860AULL};
        case GTA_Native_Id::SetPedCanRagdoll:
            return GTA_Native_Definition{id, "SET_PED_CAN_RAGDOLL", 0xB128377056A54E2AULL, 0x9FF00EA9A61211D2ULL};
        case GTA_Native_Id::ClearPedBloodDamage:
            return GTA_Native_Definition{id, "CLEAR_PED_BLOOD_DAMAGE", 0x8FE22675A5A45817ULL, 0x8EA9C5E0178372E1ULL};
        case GTA_Native_Id::ClearPedWetness:
            return GTA_Native_Definition{id, "CLEAR_PED_WETNESS", 0x9C720776DAA43E7EULL, 0x5EF96FB2D3902DC7ULL};
        case GTA_Native_Id::ClearPedEnvDirt:
            return GTA_Native_Definition{id, "CLEAR_PED_ENV_DIRT", 0x6585D955A68452A5ULL, 0xD81F5EA29FD2682EULL};
        case GTA_Native_Id::ResetPedVisibleDamage:
            return GTA_Native_Definition{id, "RESET_PED_VISIBLE_DAMAGE", 0x3AC1F7B898F30C05ULL, 0x69AE13B08EFD8497ULL};
        case GTA_Native_Id::SetPedInfiniteAmmo:
            return GTA_Native_Definition{id, "SET_PED_INFINITE_AMMO", 0x3EDCB0505123623BULL, 0xA83DA0A0DF32920CULL};
        case GTA_Native_Id::GiveWeaponToPed:
            return GTA_Native_Definition{id, "GIVE_WEAPON_TO_PED", 0xBF0FD6E56C964FCBULL, 0xB41DEC3AAC1AA107ULL};
        case GTA_Native_Id::SetPedAmmo:
            return GTA_Native_Definition{id, "SET_PED_AMMO", 0x14E56BC5B5DB6A19ULL, 0x45FC566246B3511BULL};
        case GTA_Native_Id::GetVehiclePedIsIn:
            return GTA_Native_Definition{id, "GET_VEHICLE_PED_IS_IN", 0x9A9112A0FE9A4713ULL, 0x6EF03BE64E058E2FULL};
        case GTA_Native_Id::IsWaypointActive:
            return GTA_Native_Definition{id, "IS_WAYPOINT_ACTIVE", 0x1DD1F58F493F1DA5ULL, 0x02213DC34A224533ULL};
        case GTA_Native_Id::GetWaypointBlipEnumId:
            return GTA_Native_Definition{id, "GET_WAYPOINT_BLIP_ENUM_ID", 0x186E5D252FA50E7DULL, 0x2A3612A4B836469EULL};
        case GTA_Native_Id::GetClosestBlipInfoId:
            return GTA_Native_Definition{id, "GET_CLOSEST_BLIP_INFO_ID", 0xD484BF71050CA1EEULL, 0xB981254932E1095EULL};
        case GTA_Native_Id::GetBlipCoords:
            return GTA_Native_Definition{id, "GET_BLIP_COORDS", 0x586AFE3FF72D996EULL, 0x3CF9D442F2C902BDULL};
        case GTA_Native_Id::RequestCollisionAtCoord:
            return GTA_Native_Definition{id, "REQUEST_COLLISION_AT_COORD", 0x07503F7948F491A7ULL, 0xEA2D52183C7EA9CFULL};
        case GTA_Native_Id::GetGroundZFor3DCoord:
            return GTA_Native_Definition{id, "GET_GROUND_Z_FOR_3D_COORD", 0xC906A7DAB05C8D2BULL, 0xB1EAADCB692D69CEULL};
        case GTA_Native_Id::GetWaterHeight:
            return GTA_Native_Definition{id, "GET_WATER_HEIGHT", 0xF6829842C06AE524ULL, 0xF85C2BE613AD7903ULL};
        case GTA_Native_Id::GetApproxHeightForPoint:
            return GTA_Native_Definition{id, "GET_APPROX_HEIGHT_FOR_POINT", 0x29C24BFBED8AB8FBULL, 0x54D01A0F98391D5BULL};
        case GTA_Native_Id::SetEntityCoordsNoOffset:
            return GTA_Native_Definition{id, "SET_ENTITY_COORDS_NO_OFFSET", 0x239A3351AC1DA385ULL, 0x62C438C53BB57AFDULL};
        case GTA_Native_Id::DisableAllControlActions:
            return GTA_Native_Definition{id, "DISABLE_ALL_CONTROL_ACTIONS", 0x5F4B6931816E599BULL, 0xD4510218399ED105ULL};
        default:
            return std::nullopt;
        }
    }
};
}
