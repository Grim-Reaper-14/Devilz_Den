#include "Integrations/GTA5_Enhanced/Runtime/GTA_Career_Progress_Unlocks.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>

namespace
{
using namespace Devilz::Integrations::GTA5_Enhanced;

bool TestCatalogLayout()
{
    if (GTA_Career_Progress_Entries.size() != GTA_Career_Progress_Count ||
        GTA_Career_Progress_All_Stat_Masks.size() != 5 ||
        GTA_Career_Progress_Completion_Bit_Count != 144) {
        std::cerr << "Career Progress catalog dimensions changed unexpectedly\n";
        return false;
    }

    std::array<std::uint32_t, GTA_Career_Progress_All_Stat_Masks.size()> combined{};
    for (const auto& entry : GTA_Career_Progress_Entries) {
        if (std::popcount(entry.completionMask) != GTA_Career_Progress_Tiers_Per_Career) {
            std::cerr << "Career entry does not own exactly four completion bits: "
                      << entry.name << '\n';
            return false;
        }

        bool matched = false;
        for (std::size_t i = 0; i < GTA_Career_Progress_All_Stat_Masks.size(); ++i) {
            if (GTA_Career_Progress_All_Stat_Masks[i].statIndex != entry.statIndex)
                continue;
            if ((combined[i] & entry.completionMask) != 0U) {
                std::cerr << "Career completion masks overlap for stat "
                          << entry.statIndex << '\n';
                return false;
            }
            combined[i] |= entry.completionMask;
            matched = true;
            break;
        }
        if (!matched) {
            std::cerr << "Career entry references an unknown raw stat field\n";
            return false;
        }
    }

    for (std::size_t i = 0; i < combined.size(); ++i) {
        if (combined[i] != GTA_Career_Progress_All_Stat_Masks[i].completionMask) {
            std::cerr << "Combined Career masks no longer match the verified field mask\n";
            return false;
        }
    }
    return true;
}

bool TestTierDecoding()
{
    for (const auto& entry : GTA_Career_Progress_Entries) {
        if (GTACareerProgressCompletedTierCount(entry, 0U) != 0 ||
            GTACareerProgressIsComplete(entry, 0U)) {
            std::cerr << "Empty Career field decoded as complete\n";
            return false;
        }

        if (GTACareerProgressCompletedTierCount(entry, entry.completionMask) !=
                GTA_Career_Progress_Tiers_Per_Career ||
            !GTACareerProgressIsComplete(entry, entry.completionMask)) {
            std::cerr << "Full Career mask did not decode as four completed tiers\n";
            return false;
        }

        const auto firstBit = entry.completionMask & (~entry.completionMask + 1U);
        if (GTACareerProgressCompletedTierCount(entry, firstBit) != 1 ||
            GTACareerProgressIsComplete(entry, firstBit)) {
            std::cerr << "Single Career completion bit did not decode as one tier\n";
            return false;
        }
    }
    return true;
}

bool TestRequestEncoding()
{
    if (GTACareerProgressRequestName(16801) != "__DD_CHARSTAT_INT_OR:16801" ||
        GTACareerProgressMaskValue(0xFFFFFFFFU) != "4294967295") {
        std::cerr << "Career Progress queue encoding changed unexpectedly\n";
        return false;
    }
    return true;
}
}

int main()
{
    if (!TestCatalogLayout() || !TestTierDecoding() || !TestRequestEncoding())
        return 1;

    std::cout << "GTA Career Progress catalog tests passed\n";
    return 0;
}
