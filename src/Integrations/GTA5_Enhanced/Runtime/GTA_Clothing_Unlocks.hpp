#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace Devilz::Integrations::GTA5_Enhanced
{
struct GTA_Packed_Stat_Range
{
    std::int32_t first = 0;
    std::int32_t last = 0;
};

struct GTA_Clothing_Unlock_Group
{
    std::string_view name;
    std::string_view internalTag;
    std::span<const GTA_Packed_Stat_Range> ranges;
    std::size_t expectedCount = 0;
};

inline constexpr std::string_view GTA_Clothing_Unlock_Source_Version =
    "Enhanced 1.73 / b1158.13";
inline constexpr std::size_t GTA_Clothing_Unlock_Unique_Count = 1'418;

// Verified from clothes_shop_mp.c collection and special apparel lock predicates.
// Each range is inclusive. Sparse reward systems deliberately remain sparse so
// unrelated packed stats are never changed by a clothing preset.
inline constexpr std::array LastTeamStandingClothingRanges{
    GTA_Packed_Stat_Range{3616, 3616},
};

inline constexpr std::array IndependenceDayClothingRanges{
    GTA_Packed_Stat_Range{3594, 3599},
    GTA_Packed_Stat_Range{3606, 3606},
    GTA_Packed_Stat_Range{27085, 27086},
};

inline constexpr std::array FestiveClothingRanges{
    GTA_Packed_Stat_Range{110, 112},
    GTA_Packed_Stat_Range{3750, 3750},
    GTA_Packed_Stat_Range{4333, 4335},
    GTA_Packed_Stat_Range{27088, 27088},
};

inline constexpr std::array OriginalHeistsClothingRanges{
    GTA_Packed_Stat_Range{3756, 3760},
    GTA_Packed_Stat_Range{54576, 54579},
};

inline constexpr std::array LowridersClothingRanges{
    GTA_Packed_Stat_Range{4247, 4256},
};

inline constexpr std::array FinanceAndFelonyClothingRanges{
    GTA_Packed_Stat_Range{7482, 7495},
    GTA_Packed_Stat_Range{7515, 7528},
    GTA_Packed_Stat_Range{9441, 9442},
    GTA_Packed_Stat_Range{41779, 41779},
};

inline constexpr std::array CunningStuntsClothingRanges{
    GTA_Packed_Stat_Range{7595, 7601},
    GTA_Packed_Stat_Range{26977, 26977},
};

inline constexpr std::array BikersClothingRanges{
    GTA_Packed_Stat_Range{9375, 9383},
    GTA_Packed_Stat_Range{9386, 9386},
    GTA_Packed_Stat_Range{27076, 27076},
    GTA_Packed_Stat_Range{41775, 41775},
};

inline constexpr std::array ImportExportClothingRanges{
    GTA_Packed_Stat_Range{9443, 9443},
    GTA_Packed_Stat_Range{27017, 27017},
    GTA_Packed_Stat_Range{27087, 27087},
};

inline constexpr std::array GunrunningClothingRanges{
    GTA_Packed_Stat_Range{15388, 15389},
    GTA_Packed_Stat_Range{15391, 15391},
    GTA_Packed_Stat_Range{15393, 15393},
    GTA_Packed_Stat_Range{15396, 15398},
    GTA_Packed_Stat_Range{15400, 15401},
    GTA_Packed_Stat_Range{15403, 15403},
    GTA_Packed_Stat_Range{15405, 15405},
    GTA_Packed_Stat_Range{15408, 15409},
    GTA_Packed_Stat_Range{15411, 15413},
    GTA_Packed_Stat_Range{15417, 15418},
    GTA_Packed_Stat_Range{15425, 15425},
    GTA_Packed_Stat_Range{41760, 41760},
};

inline constexpr std::array DoomsdayHeistClothingRanges{
    GTA_Packed_Stat_Range{18121, 18125},
    GTA_Packed_Stat_Range{18134, 18137},
};

inline constexpr std::array ArenaWarClothingRanges{
    GTA_Packed_Stat_Range{24970, 24970},
    GTA_Packed_Stat_Range{24977, 24977},
    GTA_Packed_Stat_Range{25000, 25000},
    GTA_Packed_Stat_Range{25004, 25005},
    GTA_Packed_Stat_Range{25018, 25023},
    GTA_Packed_Stat_Range{25025, 25031},
    GTA_Packed_Stat_Range{25178, 25178},
    GTA_Packed_Stat_Range{25225, 25225},
    GTA_Packed_Stat_Range{25244, 25371},
    GTA_Packed_Stat_Range{25373, 25375},
    GTA_Packed_Stat_Range{25377, 25379},
    GTA_Packed_Stat_Range{25382, 25383},
    GTA_Packed_Stat_Range{25386, 25386},
    GTA_Packed_Stat_Range{25390, 25393},
    GTA_Packed_Stat_Range{28171, 28171},
    GTA_Packed_Stat_Range{28173, 28175},
};

inline constexpr std::array DiamondCasinoClothingRanges{
    GTA_Packed_Stat_Range{26968, 26969},
    GTA_Packed_Stat_Range{27184, 27213},
};

inline constexpr std::array DiamondCasinoHeistClothingRanges{
    GTA_Packed_Stat_Range{28172, 28172},
    GTA_Packed_Stat_Range{28185, 28188},
    GTA_Packed_Stat_Range{28197, 28198},
    GTA_Packed_Stat_Range{28200, 28203},
    GTA_Packed_Stat_Range{28205, 28205},
    GTA_Packed_Stat_Range{28208, 28220},
    GTA_Packed_Stat_Range{28222, 28222},
    GTA_Packed_Stat_Range{28224, 28227},
    GTA_Packed_Stat_Range{28229, 28230},
    GTA_Packed_Stat_Range{28232, 28248},
    GTA_Packed_Stat_Range{41675, 41675},
    GTA_Packed_Stat_Range{41995, 41995},
    GTA_Packed_Stat_Range{42290, 42290},
    GTA_Packed_Stat_Range{51298, 51298},
    GTA_Packed_Stat_Range{54713, 54713},
    GTA_Packed_Stat_Range{54767, 54767},
};

inline constexpr std::array SummerSpecialClothingRanges{
    GTA_Packed_Stat_Range{28254, 28255},
    GTA_Packed_Stat_Range{30240, 30240},
    GTA_Packed_Stat_Range{30258, 30259},
    GTA_Packed_Stat_Range{30290, 30295},
    GTA_Packed_Stat_Range{41914, 41914},
};

inline constexpr std::array CayoPericoClothingRanges{
    GTA_Packed_Stat_Range{30563, 30567},
    GTA_Packed_Stat_Range{30572, 30631},
    GTA_Packed_Stat_Range{30638, 30693},
};

inline constexpr std::array LosSantosTunersClothingRanges{
    GTA_Packed_Stat_Range{31736, 31736},
    GTA_Packed_Stat_Range{31766, 31767},
    GTA_Packed_Stat_Range{31776, 31777},
    GTA_Packed_Stat_Range{31779, 31790},
    GTA_Packed_Stat_Range{31805, 31808},
    GTA_Packed_Stat_Range{31826, 31828},
    GTA_Packed_Stat_Range{31830, 31830},
    GTA_Packed_Stat_Range{31832, 31833},
    GTA_Packed_Stat_Range{31835, 31835},
    GTA_Packed_Stat_Range{31837, 31838},
    GTA_Packed_Stat_Range{31840, 31840},
    GTA_Packed_Stat_Range{31842, 31843},
    GTA_Packed_Stat_Range{31845, 31845},
    GTA_Packed_Stat_Range{31847, 31848},
    GTA_Packed_Stat_Range{31850, 31850},
    GTA_Packed_Stat_Range{31852, 31853},
    GTA_Packed_Stat_Range{31855, 31855},
    GTA_Packed_Stat_Range{31857, 31858},
    GTA_Packed_Stat_Range{31860, 31860},
    GTA_Packed_Stat_Range{31862, 31863},
    GTA_Packed_Stat_Range{31865, 31865},
    GTA_Packed_Stat_Range{31867, 31868},
    GTA_Packed_Stat_Range{31870, 31870},
    GTA_Packed_Stat_Range{31872, 31875},
    GTA_Packed_Stat_Range{31877, 31880},
    GTA_Packed_Stat_Range{31882, 31885},
    GTA_Packed_Stat_Range{31887, 31890},
    GTA_Packed_Stat_Range{31892, 31895},
    GTA_Packed_Stat_Range{31897, 31900},
    GTA_Packed_Stat_Range{31902, 31903},
    GTA_Packed_Stat_Range{31905, 31905},
    GTA_Packed_Stat_Range{31907, 31908},
    GTA_Packed_Stat_Range{31910, 31910},
    GTA_Packed_Stat_Range{31912, 31913},
    GTA_Packed_Stat_Range{31915, 31915},
    GTA_Packed_Stat_Range{31917, 31918},
    GTA_Packed_Stat_Range{31920, 31920},
    GTA_Packed_Stat_Range{31922, 31923},
    GTA_Packed_Stat_Range{31925, 31925},
    GTA_Packed_Stat_Range{31927, 31928},
    GTA_Packed_Stat_Range{31930, 31930},
    GTA_Packed_Stat_Range{31932, 31932},
    GTA_Packed_Stat_Range{31935, 31935},
    GTA_Packed_Stat_Range{31937, 31937},
    GTA_Packed_Stat_Range{31940, 31940},
    GTA_Packed_Stat_Range{31942, 31942},
    GTA_Packed_Stat_Range{31945, 31945},
    GTA_Packed_Stat_Range{31947, 31947},
    GTA_Packed_Stat_Range{31950, 31950},
    GTA_Packed_Stat_Range{31952, 31952},
    GTA_Packed_Stat_Range{31955, 31955},
    GTA_Packed_Stat_Range{31957, 31957},
    GTA_Packed_Stat_Range{31960, 31960},
    GTA_Packed_Stat_Range{31962, 31962},
    GTA_Packed_Stat_Range{31965, 31965},
    GTA_Packed_Stat_Range{31967, 31967},
    GTA_Packed_Stat_Range{31970, 31970},
    GTA_Packed_Stat_Range{31972, 31972},
    GTA_Packed_Stat_Range{31975, 31975},
    GTA_Packed_Stat_Range{31977, 31977},
    GTA_Packed_Stat_Range{31980, 31980},
    GTA_Packed_Stat_Range{31982, 31982},
    GTA_Packed_Stat_Range{31985, 31985},
    GTA_Packed_Stat_Range{31987, 31987},
    GTA_Packed_Stat_Range{31990, 31990},
    GTA_Packed_Stat_Range{31992, 31992},
    GTA_Packed_Stat_Range{31995, 31995},
    GTA_Packed_Stat_Range{31997, 31997},
    GTA_Packed_Stat_Range{32000, 32000},
    GTA_Packed_Stat_Range{32002, 32002},
    GTA_Packed_Stat_Range{32005, 32005},
    GTA_Packed_Stat_Range{32007, 32007},
    GTA_Packed_Stat_Range{32010, 32010},
    GTA_Packed_Stat_Range{32012, 32012},
    GTA_Packed_Stat_Range{32015, 32015},
    GTA_Packed_Stat_Range{32017, 32018},
    GTA_Packed_Stat_Range{32020, 32023},
    GTA_Packed_Stat_Range{32025, 32028},
    GTA_Packed_Stat_Range{32030, 32033},
    GTA_Packed_Stat_Range{32035, 32038},
    GTA_Packed_Stat_Range{32040, 32043},
    GTA_Packed_Stat_Range{32045, 32048},
    GTA_Packed_Stat_Range{32050, 32053},
    GTA_Packed_Stat_Range{32055, 32058},
    GTA_Packed_Stat_Range{32060, 32063},
    GTA_Packed_Stat_Range{32065, 32074},
    GTA_Packed_Stat_Range{32084, 32084},
    GTA_Packed_Stat_Range{32094, 32094},
    GTA_Packed_Stat_Range{32104, 32104},
    GTA_Packed_Stat_Range{32114, 32114},
    GTA_Packed_Stat_Range{32124, 32124},
    GTA_Packed_Stat_Range{32134, 32134},
    GTA_Packed_Stat_Range{32144, 32144},
    GTA_Packed_Stat_Range{32154, 32154},
    GTA_Packed_Stat_Range{32164, 32164},
    GTA_Packed_Stat_Range{32174, 32174},
    GTA_Packed_Stat_Range{32224, 32224},
    GTA_Packed_Stat_Range{32273, 32273},
    GTA_Packed_Stat_Range{32275, 32275},
    GTA_Packed_Stat_Range{41674, 41674},
};

inline constexpr std::array TheContractClothingRanges{
    GTA_Packed_Stat_Range{32288, 32291},
    GTA_Packed_Stat_Range{32295, 32311},
    GTA_Packed_Stat_Range{32316, 32316},
    GTA_Packed_Stat_Range{41730, 41730},
};

inline constexpr std::array Gen9LaunchClothingRanges{
    GTA_Packed_Stat_Range{34132, 34132},
    GTA_Packed_Stat_Range{34148, 34149},
};

inline constexpr std::array CriminalEnterprisesClothingRanges{
    GTA_Packed_Stat_Range{34372, 34372},
    GTA_Packed_Stat_Range{34375, 34375},
    GTA_Packed_Stat_Range{34380, 34411},
    GTA_Packed_Stat_Range{34415, 34504},
    GTA_Packed_Stat_Range{34506, 34510},
    GTA_Packed_Stat_Range{34703, 34705},
    GTA_Packed_Stat_Range{34730, 34737},
    GTA_Packed_Stat_Range{41727, 41727},
    GTA_Packed_Stat_Range{41747, 41747},
    GTA_Packed_Stat_Range{41749, 41749},
    GTA_Packed_Stat_Range{41794, 41794},
    GTA_Packed_Stat_Range{41798, 41798},
};

inline constexpr std::array DrugWarsClothingRanges{
    GTA_Packed_Stat_Range{36699, 36716},
    GTA_Packed_Stat_Range{36718, 36765},
    GTA_Packed_Stat_Range{36768, 36770},
    GTA_Packed_Stat_Range{36774, 36784},
    GTA_Packed_Stat_Range{36809, 36809},
    GTA_Packed_Stat_Range{41736, 41736},
    GTA_Packed_Stat_Range{41744, 41744},
    GTA_Packed_Stat_Range{41768, 41768},
    GTA_Packed_Stat_Range{41802, 41802},
};

inline constexpr std::array MercenariesClothingRanges{
    GTA_Packed_Stat_Range{41593, 41593},
    GTA_Packed_Stat_Range{41720, 41726},
    GTA_Packed_Stat_Range{41728, 41729},
    GTA_Packed_Stat_Range{41731, 41735},
    GTA_Packed_Stat_Range{41737, 41743},
    GTA_Packed_Stat_Range{41745, 41746},
    GTA_Packed_Stat_Range{41748, 41748},
    GTA_Packed_Stat_Range{41750, 41759},
    GTA_Packed_Stat_Range{41761, 41767},
    GTA_Packed_Stat_Range{41769, 41774},
    GTA_Packed_Stat_Range{41776, 41778},
    GTA_Packed_Stat_Range{41780, 41780},
    GTA_Packed_Stat_Range{41782, 41793},
    GTA_Packed_Stat_Range{41795, 41797},
    GTA_Packed_Stat_Range{41799, 41801},
    GTA_Packed_Stat_Range{41803, 41803},
    GTA_Packed_Stat_Range{41805, 41805},
    GTA_Packed_Stat_Range{41885, 41893},
    GTA_Packed_Stat_Range{41895, 41896},
    GTA_Packed_Stat_Range{41903, 41913},
    GTA_Packed_Stat_Range{41915, 41941},
    GTA_Packed_Stat_Range{41943, 41980},
    GTA_Packed_Stat_Range{41994, 41994},
    GTA_Packed_Stat_Range{41996, 41996},
};

inline constexpr std::array ChopShopClothingRanges{
    GTA_Packed_Stat_Range{34761, 34761},
    GTA_Packed_Stat_Range{42052, 42052},
    GTA_Packed_Stat_Range{42054, 42058},
    GTA_Packed_Stat_Range{42062, 42063},
    GTA_Packed_Stat_Range{42111, 42111},
    GTA_Packed_Stat_Range{42119, 42119},
    GTA_Packed_Stat_Range{42128, 42146},
    GTA_Packed_Stat_Range{42152, 42217},
};

inline constexpr std::array BottomDollarClothingRanges{
    GTA_Packed_Stat_Range{32407, 32408},
    GTA_Packed_Stat_Range{42257, 42257},
    GTA_Packed_Stat_Range{42268, 42268},
    GTA_Packed_Stat_Range{42286, 42287},
    GTA_Packed_Stat_Range{51215, 51223},
    GTA_Packed_Stat_Range{51225, 51258},
};

inline constexpr std::array AgentsOfSabotageClothingRanges{
    GTA_Packed_Stat_Range{32409, 32409},
    GTA_Packed_Stat_Range{42294, 42297},
    GTA_Packed_Stat_Range{54569, 54570},
    GTA_Packed_Stat_Range{54572, 54635},
    GTA_Packed_Stat_Range{54651, 54651},
};

inline constexpr std::array MoneyFrontsClothingRanges{
    GTA_Packed_Stat_Range{28319, 28321},
    GTA_Packed_Stat_Range{54664, 54664},
    GTA_Packed_Stat_Range{54682, 54707},
    GTA_Packed_Stat_Range{54711, 54712},
};

inline constexpr std::array SafehouseClothingRanges{
    GTA_Packed_Stat_Range{28344, 28345},
    GTA_Packed_Stat_Range{28351, 28351},
    GTA_Packed_Stat_Range{51365, 51378},
    GTA_Packed_Stat_Range{54769, 54772},
    GTA_Packed_Stat_Range{59971, 59971},
    GTA_Packed_Stat_Range{59978, 59980},
};

inline constexpr std::array KortzClothingRanges{
    GTA_Packed_Stat_Range{60010, 60010},
    GTA_Packed_Stat_Range{60029, 60030},
    GTA_Packed_Stat_Range{60065, 60094},
};

inline constexpr std::array CareerProgressClothingRanges{
    GTA_Packed_Stat_Range{41806, 41810},
    GTA_Packed_Stat_Range{42053, 42053},
};

inline constexpr std::array GTA_Clothing_Unlock_Groups{
    GTA_Clothing_Unlock_Group{"Last Team Standing", "LTS", LastTeamStandingClothingRanges, 1},
    GTA_Clothing_Unlock_Group{"Independence Day", "INDI", IndependenceDayClothingRanges, 9},
    GTA_Clothing_Unlock_Group{"Festive Collections", "XMAS", FestiveClothingRanges, 8},
    GTA_Clothing_Unlock_Group{"Original Heists", "HEIST", OriginalHeistsClothingRanges, 9},
    GTA_Clothing_Unlock_Group{"Lowriders", "LOW", LowridersClothingRanges, 10},
    GTA_Clothing_Unlock_Group{"Finance and Felony", "EXE", FinanceAndFelonyClothingRanges, 31},
    GTA_Clothing_Unlock_Group{"Cunning Stunts", "STUNT", CunningStuntsClothingRanges, 8},
    GTA_Clothing_Unlock_Group{"Bikers", "BIKER", BikersClothingRanges, 12},
    GTA_Clothing_Unlock_Group{"Import / Export", "IMPEXP", ImportExportClothingRanges, 3},
    GTA_Clothing_Unlock_Group{"Gunrunning", "GUNRUN", GunrunningClothingRanges, 20},
    GTA_Clothing_Unlock_Group{"Doomsday Heist", "GANGOPS", DoomsdayHeistClothingRanges, 9},
    GTA_Clothing_Unlock_Group{"Arena War", "ARENA", ArenaWarClothingRanges, 165},
    GTA_Clothing_Unlock_Group{"Diamond Casino and Resort", "VINEWOOD", DiamondCasinoClothingRanges, 32},
    GTA_Clothing_Unlock_Group{"Diamond Casino Heist", "HEIST3", DiamondCasinoHeistClothingRanges, 55},
    GTA_Clothing_Unlock_Group{"Los Santos Summer Special", "SUM", SummerSpecialClothingRanges, 12},
    GTA_Clothing_Unlock_Group{"Cayo Perico Heist", "HEIST4", CayoPericoClothingRanges, 121},
    GTA_Clothing_Unlock_Group{"Los Santos Tuners", "TUNER", LosSantosTunersClothingRanges, 187},
    GTA_Clothing_Unlock_Group{"The Contract", "FIXER", TheContractClothingRanges, 23},
    GTA_Clothing_Unlock_Group{"Expanded and Enhanced Launch", "GEN9EC", Gen9LaunchClothingRanges, 3},
    GTA_Clothing_Unlock_Group{"The Criminal Enterprises", "SUM2", CriminalEnterprisesClothingRanges, 145},
    GTA_Clothing_Unlock_Group{"Los Santos Drug Wars", "X22", DrugWarsClothingRanges, 85},
    GTA_Clothing_Unlock_Group{"San Andreas Mercenaries", "SUM23", MercenariesClothingRanges, 161},
    GTA_Clothing_Unlock_Group{"The Chop Shop", "X23", ChopShopClothingRanges, 96},
    GTA_Clothing_Unlock_Group{"Bottom Dollar Bounties", "SUM24", BottomDollarClothingRanges, 49},
    GTA_Clothing_Unlock_Group{"Agents of Sabotage", "X24", AgentsOfSabotageClothingRanges, 72},
    GTA_Clothing_Unlock_Group{"Money Fronts", "SUM25", MoneyFrontsClothingRanges, 32},
    GTA_Clothing_Unlock_Group{"A Safehouse in the Hills", "X25", SafehouseClothingRanges, 25},
    GTA_Clothing_Unlock_Group{"The Kortz Center Heist", "SUM26", KortzClothingRanges, 33},
    GTA_Clothing_Unlock_Group{"Career Progress Clothing", "CAREER", CareerProgressClothingRanges, 6},
};

[[nodiscard]] inline std::vector<std::int32_t> GTAExpandPackedStatRanges(
    std::span<const GTA_Packed_Stat_Range> ranges)
{
    std::size_t reserveCount = 0;
    for (const auto range : ranges) {
        if (range.first >= 0 && range.last >= range.first) {
            reserveCount += static_cast<std::size_t>(
                static_cast<std::int64_t>(range.last) - range.first + 1);
        }
    }

    std::vector<std::int32_t> indices;
    indices.reserve(reserveCount);
    for (const auto range : ranges) {
        if (range.first < 0 || range.last < range.first)
            continue;
        for (std::int64_t index = range.first; index <= range.last; ++index)
            indices.push_back(static_cast<std::int32_t>(index));
    }

    std::sort(indices.begin(), indices.end());
    indices.erase(std::unique(indices.begin(), indices.end()), indices.end());
    return indices;
}

[[nodiscard]] inline std::vector<std::int32_t> GTAClothingUnlockIndices(
    const GTA_Clothing_Unlock_Group& group)
{
    return GTAExpandPackedStatRanges(group.ranges);
}

[[nodiscard]] inline std::vector<std::int32_t> GTAAllClothingUnlockIndices()
{
    std::vector<std::int32_t> indices;
    indices.reserve(GTA_Clothing_Unlock_Unique_Count);
    for (const auto& group : GTA_Clothing_Unlock_Groups) {
        for (const auto range : group.ranges) {
            if (range.first < 0 || range.last < range.first)
                continue;
            for (std::int64_t index = range.first; index <= range.last; ++index)
                indices.push_back(static_cast<std::int32_t>(index));
        }
    }

    std::sort(indices.begin(), indices.end());
    indices.erase(std::unique(indices.begin(), indices.end()), indices.end());
    return indices;
}
}
