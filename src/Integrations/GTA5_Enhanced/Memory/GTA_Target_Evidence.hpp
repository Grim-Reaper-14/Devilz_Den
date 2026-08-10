#pragma once

#include "GTA_Target_Definition.hpp"

#include <cstddef>
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
    std::uint32_t candidateType = 0;
    std::uintptr_t candidateRegionBase = 0;
    std::size_t candidateRegionSize = 0;

    bool sampleRead = false;
    std::string samplePreview;
    std::size_t sampleNonZeroQwords = 0;
    std::size_t sampleReadablePointers = 0;

    bool pointerDecoded = false;
    std::uintptr_t pointeeAddress = 0;
    bool pointeeCommitted = false;
    bool pointeeReadable = false;
    bool pointeeWritable = false;
    bool pointeeExecutable = false;
    std::uint32_t pointeeProtection = 0;
    std::uint32_t pointeeType = 0;
    std::uintptr_t pointeeRegionBase = 0;
    std::size_t pointeeRegionSize = 0;

    bool pointeeSampleRead = false;
    std::string pointeeSamplePreview;
    std::size_t pointeeSampleNonZeroQwords = 0;
    std::size_t pointeeSampleReadablePointers = 0;

    std::size_t objectSlotsSampled = 0;
    std::size_t readableObjects = 0;
    std::size_t objectFirstQwordsDecoded = 0;
    std::size_t objectFirstQwordImagePointers = 0;
    std::size_t objectFirstQwordExecutableImagePointers = 0;
    std::string objectSamplePreview;

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
