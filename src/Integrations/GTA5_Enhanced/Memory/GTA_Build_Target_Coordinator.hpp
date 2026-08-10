#pragma once

#include "Backend/Error/Result.hpp"
#include "GTA_Build_Target_Registry.hpp"
#include "GTA_Target_Resolver.hpp"

#include <cstdint>
#include <vector>

namespace Devilz::Integrations::GTA5_Enhanced
{
struct GTA_Build_Target_Report
{
    std::uint64_t fingerprint = 0;
    std::vector<GTA_Target_Resolution> targets;

    [[nodiscard]] std::size_t LocatedCount() const noexcept;
    [[nodiscard]] std::size_t StructurallyValidatedCount() const noexcept;
    [[nodiscard]] std::size_t ValidatedCount() const noexcept;
};

class GTA_Build_Target_Coordinator final
{
public:
    [[nodiscard]] Devilz::Backend::Result<GTA_Build_Target_Report> Resolve(
        std::uint32_t pid,
        std::uint64_t fingerprint) const;

private:
    GTA_Build_Target_Registry m_registry;
};
}
