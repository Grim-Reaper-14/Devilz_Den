#include "Integrations/GTA5_Enhanced/Runtime/GTA_Clothing_Unlocks.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Packed_Stats_State.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string_view>

namespace
{
using namespace Devilz::Integrations::GTA5_Enhanced;

[[nodiscard]] bool Contains(
    const std::vector<std::int32_t>& indices,
    std::int32_t index)
{
    return std::binary_search(indices.begin(), indices.end(), index);
}

[[nodiscard]] const GTA_Clothing_Unlock_Group* FindGroup(std::string_view tag)
{
    const auto it = std::find_if(
        GTA_Clothing_Unlock_Groups.begin(),
        GTA_Clothing_Unlock_Groups.end(),
        [tag](const auto& group) { return group.internalTag == tag; });
    return it == GTA_Clothing_Unlock_Groups.end() ? nullptr : &*it;
}

bool TestGroupCatalog()
{
    if (GTA_Clothing_Unlock_Groups.size() != 29) {
        std::cerr << "Unexpected clothing unlock group count\n";
        return false;
    }

    for (const auto& group : GTA_Clothing_Unlock_Groups) {
        const auto indices = GTAClothingUnlockIndices(group);
        if (indices.size() != group.expectedCount) {
            std::cerr << "Packed clothing count mismatch for " << group.name
                      << ": expected " << group.expectedCount
                      << ", got " << indices.size() << '\n';
            return false;
        }
        if (!std::is_sorted(indices.begin(), indices.end()) ||
            std::adjacent_find(indices.begin(), indices.end()) != indices.end()) {
            std::cerr << "Packed clothing indices were not normalized for "
                      << group.name << '\n';
            return false;
        }
    }

    const auto* kortz = FindGroup("SUM26");
    if (!kortz) {
        std::cerr << "SUM26 clothing group is missing\n";
        return false;
    }

    const auto indices = GTAClothingUnlockIndices(*kortz);
    if (indices.size() != 33 ||
        !Contains(indices, 60010) ||
        !Contains(indices, 60029) ||
        !Contains(indices, 60030) ||
        !Contains(indices, 60065) ||
        !Contains(indices, 60094) ||
        Contains(indices, 60031)) {
        std::cerr << "SUM26 clothing unlocks do not match b1158.13\n";
        return false;
    }

    return true;
}

bool TestUnlockAllCatalog()
{
    const auto indices = GTAAllClothingUnlockIndices();
    if (indices.size() != GTA_Clothing_Unlock_Unique_Count ||
        !std::is_sorted(indices.begin(), indices.end()) ||
        std::adjacent_find(indices.begin(), indices.end()) != indices.end()) {
        std::cerr << "Unlock-all clothing catalog was not deduplicated to 1418 indices\n";
        return false;
    }

    if (Contains(indices, 3755) ||
        Contains(indices, 31825) ||
        Contains(indices, 32274)) {
        std::cerr << "Unlock-all clothing catalog contains a known non-clothing packed stat\n";
        return false;
    }

    if (!Contains(indices, 110) ||
        !Contains(indices, 41806) ||
        !Contains(indices, 54576) ||
        !Contains(indices, 60094)) {
        std::cerr << "Unlock-all clothing catalog lost a boundary or special reward\n";
        return false;
    }

    return true;
}

bool TestPackedQueueIntegration()
{
    auto& state = GTA_Packed_Stats_State::Instance();
    state.Reset();

    const auto indices = GTAAllClothingUnlockIndices();
    const auto batchId = state.RequestWrites(indices, true);
    const auto batch = state.BatchSnapshot();
    if (batch.batchId != batchId ||
        batch.total != GTA_Clothing_Unlock_Unique_Count ||
        !batch.active ||
        batch.valueType != GTA_Packed_Stat_Value_Type::Bool) {
        std::cerr << "Clothing unlock batch was not queued as packed BOOL writes\n";
        return false;
    }

    std::size_t consumed = 0;
    GTA_Packed_Stats_State::Command command{};
    while (state.Consume(command)) {
        if (command.batchId != batchId ||
            command.kind != GTA_Packed_Stats_State::Command_Kind::Write ||
            command.valueType != GTA_Packed_Stat_Value_Type::Bool ||
            command.value != 1) {
            std::cerr << "Clothing unlock command transport is incorrect\n";
            return false;
        }
        ++consumed;
    }

    state.Reset();
    if (consumed != GTA_Clothing_Unlock_Unique_Count) {
        std::cerr << "Clothing unlock queue dropped commands\n";
        return false;
    }

    return true;
}
}

int main()
{
    if (!TestGroupCatalog() ||
        !TestUnlockAllCatalog() ||
        !TestPackedQueueIntegration()) {
        return 1;
    }

    std::cout << "GTA clothing unlock catalog tests passed\n";
    return 0;
}
