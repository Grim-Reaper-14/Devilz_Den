#include "GTA_Target_Structural_Validator.hpp"

#include <Windows.h>

#include <string>

namespace Devilz::Integrations::GTA5_Enhanced
{
namespace
{
GTA_Target_Structural_Validation ValidateScriptThreads(const GTA_Target_Evidence& evidence)
{
    GTA_Target_Structural_Validation result{};
    result.applicable = true;

    if (!evidence.candidateCommitted || !evidence.candidateReadable) {
        result.detail = "ScriptThreads storage is not committed and readable";
        return result;
    }

    if (evidence.candidateType != MEM_IMAGE) {
        result.detail = "ScriptThreads storage is not backed by the GTA image";
        return result;
    }

    if (!evidence.pointerDecoded || evidence.pointeeAddress == 0) {
        result.detail = "ScriptThreads storage did not decode to a non-null collection pointer";
        return result;
    }

    if (!evidence.pointeeCommitted || !evidence.pointeeReadable) {
        result.detail = "ScriptThreads collection is not committed and readable";
        return result;
    }

    if (evidence.pointeeExecutable) {
        result.detail = "ScriptThreads collection unexpectedly points into executable memory";
        return result;
    }

    if (evidence.objectSlotsSampled < 4 || evidence.readableObjects < 4) {
        result.detail = "Too few readable ScriptThreads object entries were observed";
        return result;
    }

    if (evidence.objectFirstQwordsDecoded < 4) {
        result.detail = "Too few ScriptThreads object headers could be sampled";
        return result;
    }

    const auto executableImagePointers = evidence.objectFirstQwordExecutableImagePointers;
    const auto decodedHeaders = evidence.objectFirstQwordsDecoded;
    if ((executableImagePointers * 4) < (decodedHeaders * 3)) {
        result.detail = "Fewer than 75 percent of sampled ScriptThreads object headers point into executable image memory";
        return result;
    }

    result.passed = true;
    result.detail =
        "ScriptThreads pointer collection passed structural validation: " +
        std::to_string(evidence.readableObjects) + "/" +
        std::to_string(evidence.objectSlotsSampled) + " sampled objects are readable and " +
        std::to_string(executableImagePointers) + "/" +
        std::to_string(decodedHeaders) +
        " decoded first-qword pointers target executable image memory; semantic thread identity is still pending";
    return result;
}
}

GTA_Target_Structural_Validation GTA_Target_Structural_Validator::Validate(
    const GTA_Target_Definition& definition,
    const GTA_Target_Evidence& evidence)
{
    if (definition.id == GTA_Runtime_Target_Id::ScriptThreads &&
        definition.candidateKind == GTA_Target_Candidate_Kind::PointerStorage) {
        return ValidateScriptThreads(evidence);
    }

    return {};
}
}
