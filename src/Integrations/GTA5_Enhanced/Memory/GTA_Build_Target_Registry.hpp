#pragma once

#include "GTA_Target_Definition.hpp"

#include <cstdint>
#include <optional>
#include <unordered_map>

namespace Devilz::Integrations::GTA5_Enhanced
{
class GTA_Build_Target_Registry final
{
public:
    GTA_Build_Target_Registry();

    [[nodiscard]] std::optional<GTA_Build_Target_Set> Find(std::uint64_t fingerprint) const;

private:
    void Register(GTA_Build_Target_Set set);

    std::unordered_map<std::uint64_t, GTA_Build_Target_Set> m_sets;
};
}
