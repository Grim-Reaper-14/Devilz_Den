#pragma once

#include "GTA_Target_Definition.hpp"

#include <cstdint>
#include <string>

namespace Devilz::Integrations::GTA5_Enhanced
{
struct GTA_Target_Evidence
{
    bool candidateCommitted = false;
    bool candidateReadable = false;
    bool candidateWritable = false;
    bool candidateExecutable = false;
    std::uint32_t candidateProtection = 0;

    bool pointerDecoded = false;
    std::uintptr_t pointeeAddress = 0;
    bool pointeeCommitted = false;
    bool pointeeReadable = false;
    std::uint32_t pointeeProtection = 0;

    std::string summary;
};

class GTA_Target_Evidence_Probe final
{
public:
    [[nodiscard]] static GTA_Target_Evidence Probe(
        std::uint32_t pid,
        GTA_Target_Candidate_Kind kind,
        std::uintptr_t address);
};
}
