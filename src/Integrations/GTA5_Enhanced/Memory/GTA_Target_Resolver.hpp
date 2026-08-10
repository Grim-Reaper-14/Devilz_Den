#pragma once

#include "Backend/Error/Result.hpp"
#include "Backend/Process/Process_Module_Manager.hpp"
#include "Backend/Process/Process_Pattern_Scanner.hpp"
#include "GTA_Target_Definition.hpp"
#include "GTA_Target_Evidence.hpp"
#include "../Runtime/GTA_Runtime_Target.hpp"

#include <cstdint>
#include <vector>

namespace Devilz::Integrations::GTA5_Enhanced
{
struct GTA_Target_Resolution
{
    GTA_Runtime_Target_Status status{};
    GTA_Target_Candidate_Kind candidateKind = GTA_Target_Candidate_Kind::Unknown;
    GTA_Target_Evidence evidence{};
    std::uintptr_t address = 0;
    std::vector<std::uintptr_t> candidates;
};

class GTA_Target_Resolver final
{
public:
    explicit GTA_Target_Resolver(std::uint32_t pid) noexcept : m_pid(pid) {}

    [[nodiscard]] Devilz::Backend::Result<GTA_Target_Resolution> Resolve(
        const GTA_Target_Definition& definition) const;

private:
    std::uint32_t m_pid = 0;
};
}
