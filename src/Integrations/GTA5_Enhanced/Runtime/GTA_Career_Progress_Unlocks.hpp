#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace Devilz::Integrations::GTA5_Enhanced
{
struct GTA_Career_Progress_Entry
{
    std::int32_t careerId = 0;
    std::string_view name;
    std::string_view internalTag;
    std::int32_t statIndex = 0;
    std::uint32_t completionMask = 0;
};

struct GTA_Career_Progress_Stat_Mask
{
    std::int32_t statIndex = 0;
    std::uint32_t completionMask = 0;
};

inline constexpr std::string_view GTA_Career_Progress_Source =
    "Enhanced appprogresshub.c / func_66, func_99, func_106, func_115, func_118";
inline constexpr std::string_view GTA_Career_Progress_Request_Prefix =
    "__DD_CHARSTAT_INT_OR:";
inline constexpr std::size_t GTA_Career_Progress_Count = 36;
inline constexpr std::size_t GTA_Career_Progress_Tiers_Per_Career = 4;
inline constexpr std::size_t GTA_Career_Progress_Completion_Bit_Count =
    GTA_Career_Progress_Count * GTA_Career_Progress_Tiers_Per_Career;

inline constexpr std::array<GTA_Career_Progress_Stat_Mask, 5> GTA_Career_Progress_All_Stat_Masks{{
    {16801, 0xFFFFFFFFU},
    {16802, 0xFFFFFFFFU},
    {16803, 0xFFFFFFFFU},
    {16804, 0xFFFFFFFFU},
    {18532, 0x0000FFFFU},
}};

inline constexpr std::array<GTA_Career_Progress_Entry, GTA_Career_Progress_Count> GTA_Career_Progress_Entries{{
    {0,  "KNOWAY",                          "KNOWAY",                          18532, 0x00000F00U},
    {1,  "FIELD HANGAR",                    "FIELD_HANGAR",                    18532, 0x0000000FU},
    {2,  "CHICKEN FACTORY RAID",            "CHICKEN_FACTORY_RAID",            16804, 0x00F00000U},
    {3,  "PROJECT OVERTHROW",               "PROJECT_OVERTHROW",               16801, 0x0000000FU},
    {4,  "OPERATION PAPER TRAIL",           "OPERATION_PAPER_TRAIL",           16801, 0x000000F0U},
    {5,  "SUPERYACHT LIFE",                 "SUPERYACHT_LIFE",                 16801, 0x00000F00U},
    {6,  "GERALDS LAST PLAY",               "GERALDS_LAST_PLAY",               16801, 0x0000F000U},
    {7,  "PREMIUM DELUXE REPO WORK",        "PREMIUM_DELUXE_REPO_WORK",        16801, 0x000F0000U},
    {8,  "MADRAZO DISPATCH SERVICES",       "MADRAZO_DISPATCH_SERVICES",       16801, 0x00F00000U},
    {9,  "LOWRIDERS",                       "LOWRIDERS",                       16801, 0x0F000000U},
    {10, "BUSINESS TYCOON",                 "BUSINESS_TYCOON",                 18532, 0x000000F0U},
    {11, "HACKER DEN",                      "HACKER_DEN",                      16804, 0xF0000000U},
    {12, "BAIL OFFICE",                     "BAIL_OFFICE",                     16804, 0x0F000000U},
    {13, "SALVAGE YARD",                    "SALVAGE_YARD",                    16804, 0x000F0000U},
    {14, "LOS SANTOS DRUG WARS",            "LOS_SANTOS_DRUG_WARS",            16801, 0xF0000000U},
    {15, "THE CONTRACT",                    "THE_CONTRACT",                    16802, 0x0000000FU},
    {16, "AFTER HOURS",                     "AFTER_HOURS",                     16802, 0x000000F0U},
    {17, "SMUGGLERS RUN",                   "SMUGGLERS_RUN",                   16802, 0x00000F00U},
    {18, "GUNRUNNING",                      "GUNRUNNING",                      16802, 0x0000F000U},
    {19, "IMPORT EXPORT",                   "IMPORT_EXPORT",                   16802, 0x000F0000U},
    {20, "BIKERS",                          "BIKERS",                          16802, 0x00F00000U},
    {21, "FAIFAF",                          "FAIFAF",                          16802, 0x0F000000U},
    {22, "LOS SANTOS TUNERS",               "LOS_SANTOS_TUNERS",               16802, 0xF0000000U},
    {23, "DIAMOND CASINO",                  "DIAMOND_CASINO",                  16803, 0x0000000FU},
    {24, "KORTZ HEIST",                     "KORTZ_HEIST",                     18532, 0x0000F000U},
    {25, "CAYO PERICO HEIST",               "CAYO_PERICO_HEIST",               16803, 0x000000F0U},
    {26, "DIAMOND CASINO HEIST",            "DIAMOND_CASINO_HEIST",            16803, 0x00000F00U},
    {27, "DOOMSDAY HEIST",                  "DOOMSDAY_HEIST",                  16803, 0x0000F000U},
    {28, "HEISTS",                          "HEISTS",                          16803, 0x000F0000U},
    {29, "ARENA WAR",                       "ARENA_WAR",                       16803, 0x00F00000U},
    {30, "ADVERSARY MODES",                 "ADVERSARY_MODES",                 16803, 0x0F000000U},
    {31, "SURVIVALS",                       "SURVIVALS",                       16803, 0xF0000000U},
    {32, "RACING",                          "RACING",                          16804, 0x0000000FU},
    {33, "DEATHMATCHES",                    "DEATHMATCHES",                    16804, 0x000000F0U},
    {34, "VEHICLE ENTHUSIAST",              "VEHICLE_ENTHUSIAST",              16804, 0x00000F00U},
    {35, "WEAPONS EXPERT",                  "WEAPONS_EXPERT",                  16804, 0x0000F000U},
}};

[[nodiscard]] inline std::string GTACareerProgressRequestName(std::int32_t statIndex)
{
    return std::string{GTA_Career_Progress_Request_Prefix} + std::to_string(statIndex);
}

[[nodiscard]] inline std::string GTACareerProgressMaskValue(std::uint32_t mask)
{
    return std::to_string(static_cast<unsigned long long>(mask));
}

[[nodiscard]] consteval bool GTAValidateCareerProgressLayout()
{
    std::array<std::uint32_t, GTA_Career_Progress_All_Stat_Masks.size()> combined{};
    for (const auto& entry : GTA_Career_Progress_Entries) {
        bool found = false;
        for (std::size_t i = 0; i < GTA_Career_Progress_All_Stat_Masks.size(); ++i) {
            if (GTA_Career_Progress_All_Stat_Masks[i].statIndex != entry.statIndex)
                continue;
            if ((combined[i] & entry.completionMask) != 0U)
                return false;
            combined[i] |= entry.completionMask;
            found = true;
            break;
        }
        if (!found || entry.completionMask == 0U)
            return false;
    }

    for (std::size_t i = 0; i < combined.size(); ++i) {
        if (combined[i] != GTA_Career_Progress_All_Stat_Masks[i].completionMask)
            return false;
    }
    return true;
}

static_assert(GTAValidateCareerProgressLayout());
}
