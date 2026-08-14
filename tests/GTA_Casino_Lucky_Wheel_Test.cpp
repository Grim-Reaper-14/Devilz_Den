#include "Integrations/GTA5_Enhanced/Runtime/GTA_Casino_Extension.hpp"

#include <array>
#include <cstddef>
#include <iostream>
#include <string_view>

using namespace Devilz::Integrations::GTA5_Enhanced;

int main()
{
    if (GTA_Lucky_Wheel_Max_Spins_Global != 289000U ||
        GTA_Lucky_Wheel_Additional_Spins_Global != 289001U ||
        GTA_Lucky_Wheel_Gta_Plus_Max_Spins_Global != 299603U) {
        std::cerr << "Lucky Wheel tunable globals do not match Enhanced 1.73\n";
        return 1;
    }

    if (GTA_Lucky_Wheel_Outcome_Local_Index(0) != 151U ||
        GTA_Lucky_Wheel_Outcome_Local_Index(31) != 306U ||
        GTA_Lucky_Wheel_Outcome_Local_Index(31) + 4U != 310U) {
        std::cerr << "Lucky Wheel SCR_ARRAY player-stride layout is incorrect\n";
        return 2;
    }

    if (GTA_Lucky_Wheel_Outcomes.size() != 20U ||
        std::string_view{GTA_Lucky_Wheel_Outcome_Name(18)} != "Podium Vehicle" ||
        std::string_view{GTA_Lucky_Wheel_Outcome_Name(-1)} != "Not selected") {
        std::cerr << "Lucky Wheel outcome labels are incomplete\n";
        return 3;
    }

    constexpr std::array<int, GTA_Lucky_Wheel_Outcome_Count> expectedAmounts{
        0, 2500, 20000, 10000, 0,
        5000, 30000, 15000, 0, 7500,
        20000, 0, 0, 10000, 40000,
        25000, 0, 15000, 0, 50000
    };
    std::array<int, 7> rewardKindCounts{};
    for (std::size_t index = 0; index < GTA_Lucky_Wheel_Outcomes.size(); ++index) {
        const auto& outcome = GTA_Lucky_Wheel_Outcomes[index];
        if (outcome.amount != expectedAmounts[index] || !outcome.name || *outcome.name == '\0') {
            std::cerr << "Lucky Wheel outcome definition mismatch at index " << index << '\n';
            return 4;
        }

        ++rewardKindCounts[static_cast<std::size_t>(outcome.kind)];
    }

    constexpr std::array<int, 7> expectedKindCounts{4, 5, 4, 4, 1, 1, 1};
    if (rewardKindCounts != expectedKindCounts) {
        std::cerr << "Lucky Wheel prize category distribution is incorrect\n";
        return 5;
    }

    return 0;
}
