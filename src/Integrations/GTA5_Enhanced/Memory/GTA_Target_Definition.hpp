#pragma once

#include "GTA_Address_Resolver.hpp"
#include "../Runtime/GTA_Runtime_Target.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace Devilz::Integrations::GTA5_Enhanced
{
enum class GTA_Target_Candidate_Kind : std::uint8_t
{
    Unknown,
    DirectData,
    PointerStorage,
    CodeSite
};

struct GTA_Target_Definition
{
    GTA_Runtime_Target_Id id = GTA_Runtime_Target_Id::GameState;
    std::string name;
    std::string module = "GTA5_Enhanced.exe";
    std::string pattern;
    GTA_Address_Resolve_Chain resolve;
    bool required = true;
    GTA_Target_Candidate_Kind candidateKind = GTA_Target_Candidate_Kind::Unknown;
};

struct GTA_Build_Target_Set
{
    std::uint64_t fingerprint = 0;
    std::vector<GTA_Target_Definition> targets;
};
}
