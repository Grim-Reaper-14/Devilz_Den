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
struct GTA_Collection_Unlock_Group
{
    std::string_view name;
    std::string_view description;
    std::span<const std::int32_t> indices;
};

inline constexpr std::string_view GTA_Collection_Unlock_Source =
    "YimMenuV2 enhanced / DailyActivities.cpp";
inline constexpr std::size_t GTA_Collection_Unlock_Unique_Count = 37;

// Verified against YimMenuV2's Enhanced DailyActivities completion path.
// These are progress flags, not live event/tunable overrides. GTA may reset
// daily-scoped flags during its normal daily/weekly rotation.
inline constexpr std::array GTA_Gs_Cache_Collection_Indices{
    std::int32_t{36628},
};

inline constexpr std::array GTA_Stash_House_Collection_Indices{
    std::int32_t{36657},
};

inline constexpr std::array GTA_Shipwreck_Collection_Indices{
    std::int32_t{31734},
};

inline constexpr std::array GTA_Hidden_Cache_Collection_Indices{
    std::int32_t{30297}, std::int32_t{30298}, std::int32_t{30299}, std::int32_t{30300},
    std::int32_t{30301}, std::int32_t{30302}, std::int32_t{30303}, std::int32_t{30304},
    std::int32_t{30305}, std::int32_t{30306},
};

inline constexpr std::array GTA_Treasure_Chest_Collection_Indices{
    std::int32_t{30307}, std::int32_t{30308},
};

inline constexpr std::array GTA_Buried_Stash_Collection_Indices{
    std::int32_t{25522}, std::int32_t{25523},
};

inline constexpr std::array GTA_LS_Tag_Collection_Indices{
    std::int32_t{42252}, std::int32_t{42253}, std::int32_t{42254},
    std::int32_t{42255}, std::int32_t{42256},
};

inline constexpr std::array GTA_Madrazo_Hit_Collection_Indices{
    std::int32_t{42269},
};

inline constexpr std::array GTA_Wildlife_Photography_Collection_Indices{
    std::int32_t{42059}, std::int32_t{42060}, std::int32_t{42061},
};

inline constexpr std::array GTA_Smoke_On_The_Water_Collection_Indices{
    std::int32_t{54672}, std::int32_t{54673}, std::int32_t{54674}, std::int32_t{54675},
    std::int32_t{54676}, std::int32_t{54677}, std::int32_t{54678}, std::int32_t{54679},
    std::int32_t{54680}, std::int32_t{54681},
};

inline constexpr std::array GTA_Golden_Clover_Collection_Indices{
    std::int32_t{54735},
};

inline constexpr std::array GTA_Collection_Unlock_Groups{
    GTA_Collection_Unlock_Group{
        "G's Cache",
        "Marks today's G's Cache completion flag.",
        GTA_Gs_Cache_Collection_Indices,
    },
    GTA_Collection_Unlock_Group{
        "Stash House",
        "Marks today's Stash House completion flag.",
        GTA_Stash_House_Collection_Indices,
    },
    GTA_Collection_Unlock_Group{
        "Shipwreck",
        "Marks the Shipwreck daily collection flag.",
        GTA_Shipwreck_Collection_Indices,
    },
    GTA_Collection_Unlock_Group{
        "Hidden Caches",
        "Marks all ten Hidden Cache collection flags.",
        GTA_Hidden_Cache_Collection_Indices,
    },
    GTA_Collection_Unlock_Group{
        "Treasure Chests",
        "Marks both Treasure Chest collection flags.",
        GTA_Treasure_Chest_Collection_Indices,
    },
    GTA_Collection_Unlock_Group{
        "Buried Stashes",
        "Marks both Buried Stash collection flags.",
        GTA_Buried_Stash_Collection_Indices,
    },
    GTA_Collection_Unlock_Group{
        "LS Tags",
        "Marks all five LS Tag completion flags.",
        GTA_LS_Tag_Collection_Indices,
    },
    GTA_Collection_Unlock_Group{
        "Madrazo Hit",
        "Marks the Madrazo Hit daily completion flag.",
        GTA_Madrazo_Hit_Collection_Indices,
    },
    GTA_Collection_Unlock_Group{
        "Wildlife Photography",
        "Marks all three daily wildlife photography flags.",
        GTA_Wildlife_Photography_Collection_Indices,
    },
    GTA_Collection_Unlock_Group{
        "Smoke on the Water Products",
        "Marks all ten product collection flags.",
        GTA_Smoke_On_The_Water_Collection_Indices,
    },
    GTA_Collection_Unlock_Group{
        "Golden Clover",
        "Marks the Golden Clover collection flag.",
        GTA_Golden_Clover_Collection_Indices,
    },
};

inline std::vector<std::int32_t> GTACollectionUnlockIndices(
    const GTA_Collection_Unlock_Group& group)
{
    return {group.indices.begin(), group.indices.end()};
}

inline std::vector<std::int32_t> GTAAllCollectionUnlockIndices()
{
    std::vector<std::int32_t> indices;
    indices.reserve(GTA_Collection_Unlock_Unique_Count);
    for (const auto& group : GTA_Collection_Unlock_Groups)
        indices.insert(indices.end(), group.indices.begin(), group.indices.end());

    std::sort(indices.begin(), indices.end());
    indices.erase(std::unique(indices.begin(), indices.end()), indices.end());
    return indices;
}
}
