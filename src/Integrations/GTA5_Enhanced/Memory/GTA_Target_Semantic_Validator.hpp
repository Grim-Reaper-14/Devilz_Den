#pragma once

#include "GTA_Target_Definition.hpp"
#include "GTA_Target_Evidence.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <sstream>
#include <string>

namespace Devilz::Integrations::GTA5_Enhanced
{
struct GTA_Target_Semantic_Validation
{
    bool applicable = false;
    bool passed = false;
    std::string detail;
};

class GTA_Target_Semantic_Validator final
{
public:
    static constexpr std::uint64_t SupportedFingerprint = 0x6A4F97F605B81000ULL;
    static constexpr std::uintptr_t ScriptThreadsDispatchTableRva = 0x2679270ULL;
    static constexpr std::uintptr_t RunScriptThreadsEntryRva = 0x920A40ULL;
    static constexpr std::uintptr_t InitNativeTablesEntryRva = 0x91BF20ULL;
    static constexpr const char* RunScriptThreadsEntryBytes =
        "56 57 55 53 48 83 EC 28 85 C9 BE 40 5D C6 00 0F";

    [[nodiscard]] static GTA_Target_Semantic_Validation Validate(
        const GTA_Target_Definition& definition,
        const GTA_Target_Evidence& evidence,
        std::uint64_t fingerprint,
        std::uintptr_t moduleBase,
        std::uintptr_t candidateAddress)
    {
        GTA_Target_Semantic_Validation result{};

        if (definition.id == GTA_Runtime_Target_Id::ScriptThreads &&
            definition.candidateKind == GTA_Target_Candidate_Kind::PointerStorage) {
            result.applicable = true;

            if (fingerprint != SupportedFingerprint) {
                result.detail = "No ScriptThreads semantic identity is registered for this GTA build fingerprint";
                return result;
            }

            if (moduleBase == 0 ||
                moduleBase > (std::numeric_limits<std::uintptr_t>::max)() - ScriptThreadsDispatchTableRva) {
                result.detail = "GTA module base is unavailable for ScriptThreads semantic validation";
                return result;
            }

            if (evidence.objectFirstQwordsDecoded < 4 ||
                evidence.objectDominantFirstQwordCount != evidence.objectFirstQwordsDecoded) {
                result.detail = "Sampled ScriptThreads objects do not share one dominant dispatch-table identity";
                return result;
            }

            if (!evidence.objectDominantFirstQwordSampleRead) {
                result.detail = "ScriptThreads dominant dispatch table could not be sampled";
                return result;
            }

            const auto expectedDispatchTable = moduleBase + ScriptThreadsDispatchTableRva;
            if (evidence.objectDominantFirstQwordAddress != expectedDispatchTable) {
                result.detail = "ScriptThreads dominant dispatch-table RVA does not match the verified build identity";
                return result;
            }

            constexpr std::size_t requiredDispatchEntries = 8;
            if (evidence.objectDominantFirstQwordReadablePointers < requiredDispatchEntries ||
                evidence.objectDominantFirstQwordExecutableImagePointers < requiredDispatchEntries) {
                result.detail = "ScriptThreads dispatch table does not expose eight readable executable-image entries";
                return result;
            }

            std::ostringstream detail;
            detail << "ScriptThreads semantic identity validated for fingerprint 0x"
                   << std::uppercase << std::hex << fingerprint
                   << ": " << std::dec << evidence.objectDominantFirstQwordCount
                   << "/" << evidence.objectFirstQwordsDecoded
                   << " sampled objects share dispatch-table RVA 0x"
                   << std::uppercase << std::hex << ScriptThreadsDispatchTableRva
                   << " and 8/8 sampled dispatch entries target executable GTA image memory";

            result.passed = true;
            result.detail = detail.str();
            return result;
        }

        if (definition.id == GTA_Runtime_Target_Id::RunScriptThreads &&
            definition.candidateKind == GTA_Target_Candidate_Kind::CodeSite) {
            result.applicable = true;

            if (fingerprint != SupportedFingerprint) {
                result.detail = "No RunScriptThreads semantic identity is registered for this GTA build fingerprint";
                return result;
            }

            if (moduleBase == 0 ||
                moduleBase > (std::numeric_limits<std::uintptr_t>::max)() - RunScriptThreadsEntryRva) {
                result.detail = "GTA module base is unavailable for RunScriptThreads semantic validation";
                return result;
            }

            const auto expectedEntry = moduleBase + RunScriptThreadsEntryRva;
            if (candidateAddress != expectedEntry) {
                result.detail = "RunScriptThreads callable-entry RVA does not match the verified build identity";
                return result;
            }

            if (!evidence.candidateCommitted || !evidence.candidateReadable || !evidence.candidateExecutable ||
                evidence.candidateType != 0x01000000U) {
                result.detail = "RunScriptThreads candidate is not committed readable executable image memory";
                return result;
            }

            if (!evidence.sampleRead || evidence.samplePreview != RunScriptThreadsEntryBytes) {
                result.detail = "RunScriptThreads callable-entry bytes do not match the verified build identity";
                return result;
            }

            std::ostringstream detail;
            detail << "RunScriptThreads callable entry validated for fingerprint 0x"
                   << std::uppercase << std::hex << fingerprint
                   << " at RVA 0x" << RunScriptThreadsEntryRva;

            result.passed = true;
            result.detail = detail.str();
            return result;
        }

        if (definition.id == GTA_Runtime_Target_Id::InitNativeTables &&
            definition.candidateKind == GTA_Target_Candidate_Kind::CodeSite) {
            result.applicable = true;

            if (fingerprint != SupportedFingerprint) {
                result.detail = "No InitNativeTables semantic identity is registered for this GTA build fingerprint";
                return result;
            }

            if (moduleBase == 0 ||
                moduleBase > (std::numeric_limits<std::uintptr_t>::max)() - InitNativeTablesEntryRva) {
                result.detail = "GTA module base is unavailable for InitNativeTables semantic validation";
                return result;
            }

            const auto expectedEntry = moduleBase + InitNativeTablesEntryRva;
            if (candidateAddress != expectedEntry) {
                result.detail = "InitNativeTables callable-entry RVA does not match the verified build identity";
                return result;
            }

            if (!evidence.candidateCommitted || !evidence.candidateReadable || !evidence.candidateExecutable) {
                result.detail = "InitNativeTables candidate is not committed readable executable image memory";
                return result;
            }

            if (!evidence.sampleRead || evidence.samplePreview.empty()) {
                result.detail = "InitNativeTables callable entry could not be sampled";
                return result;
            }

            std::ostringstream detail;
            detail << "InitNativeTables callable bootstrap validated for fingerprint 0x"
                   << std::uppercase << std::hex << fingerprint
                   << " at RVA 0x" << InitNativeTablesEntryRva;

            result.passed = true;
            result.detail = detail.str();
            return result;
        }

        return result;
    }
};
}
