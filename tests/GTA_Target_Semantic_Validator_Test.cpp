#include "Integrations/GTA5_Enhanced/Memory/GTA_Target_Semantic_Validator.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Build_Registry.hpp"

#include <cstdint>
#include <iostream>
#include <vector>

namespace
{
using namespace Devilz::Integrations::GTA5_Enhanced;

GTA_Target_Definition ScriptGlobalsDefinition()
{
    GTA_Target_Definition definition{};
    definition.id = GTA_Runtime_Target_Id::ScriptGlobals;
    definition.name = "ScriptGlobals";
    definition.candidateKind = GTA_Target_Candidate_Kind::DirectData;
    return definition;
}

GTA_Target_Definition ScriptThreadsDefinition()
{
    GTA_Target_Definition definition{};
    definition.id = GTA_Runtime_Target_Id::ScriptThreads;
    definition.name = "ScriptThreads";
    definition.candidateKind = GTA_Target_Candidate_Kind::PointerStorage;
    return definition;
}

GTA_Target_Definition RunScriptThreadsDefinition()
{
    GTA_Target_Definition definition{};
    definition.id = GTA_Runtime_Target_Id::RunScriptThreads;
    definition.name = "RunScriptThreads";
    definition.candidateKind = GTA_Target_Candidate_Kind::CodeSite;
    return definition;
}

GTA_Target_Definition InitNativeTablesDefinition()
{
    GTA_Target_Definition definition{};
    definition.id = GTA_Runtime_Target_Id::InitNativeTables;
    definition.name = "InitNativeTables";
    definition.candidateKind = GTA_Target_Candidate_Kind::CodeSite;
    return definition;
}

GTA_Target_Evidence VerifiedScriptGlobalsEvidence()
{
    GTA_Target_Evidence evidence{};
    evidence.candidateCommitted = true;
    evidence.candidateReadable = true;
    evidence.candidateWritable = true;
    evidence.candidateType = 0x01000000U;
    evidence.sampleRead = true;
    evidence.sampleNonZeroQwords = 5;
    evidence.sampleReadablePointers = 5;
    return evidence;
}

GTA_Target_Evidence VerifiedScriptThreadsEvidence(std::uintptr_t moduleBase)
{
    GTA_Target_Evidence evidence{};
    evidence.objectFirstQwordsDecoded = 8;
    evidence.objectDominantFirstQwordCount = 8;
    evidence.objectDominantFirstQwordAddress =
        moduleBase + GTA_Target_Semantic_Validator::ScriptThreadsDispatchTableRva;
    evidence.objectDominantFirstQwordSampleRead = true;
    evidence.objectDominantFirstQwordReadablePointers = 8;
    evidence.objectDominantFirstQwordExecutableImagePointers = 8;
    return evidence;
}

GTA_Target_Evidence VerifiedRunScriptThreadsEvidence()
{
    GTA_Target_Evidence evidence{};
    evidence.candidateCommitted = true;
    evidence.candidateReadable = true;
    evidence.candidateExecutable = true;
    evidence.candidateType = 0x01000000U;
    evidence.sampleRead = true;
    evidence.samplePreview = GTA_Target_Semantic_Validator::RunScriptThreadsEntryBytes;
    return evidence;
}

GTA_Target_Evidence VerifiedNativeBootstrapEvidence()
{
    GTA_Target_Evidence evidence{};
    evidence.candidateCommitted = true;
    evidence.candidateReadable = true;
    evidence.candidateExecutable = true;
    evidence.sampleRead = true;
    evidence.samplePreview = "48 89 5C 24 08";
    return evidence;
}

bool TestVerifiedScriptGlobalsIdentity()
{
    constexpr std::uintptr_t moduleBase = 0x00007FF600000000ULL;
    const auto candidate = moduleBase + GTA_Target_Semantic_Validator::ScriptGlobalsRva;
    const auto validation = GTA_Target_Semantic_Validator::Validate(
        ScriptGlobalsDefinition(),
        VerifiedScriptGlobalsEvidence(),
        GTA_Target_Semantic_Validator::SupportedFingerprint,
        moduleBase,
        candidate);

    if (!validation.applicable || !validation.passed) {
        std::cerr << "Verified ScriptGlobals semantic identity was rejected: "
                  << validation.detail << '\n';
        return false;
    }
    return true;
}

bool TestWrongScriptGlobalsRva()
{
    constexpr std::uintptr_t moduleBase = 0x00007FF600000000ULL;
    const auto candidate = moduleBase + GTA_Target_Semantic_Validator::ScriptGlobalsRva + 8;
    const auto validation = GTA_Target_Semantic_Validator::Validate(
        ScriptGlobalsDefinition(), VerifiedScriptGlobalsEvidence(),
        GTA_Target_Semantic_Validator::SupportedFingerprint, moduleBase, candidate);

    if (!validation.applicable || validation.passed) {
        std::cerr << "Wrong ScriptGlobals RVA was accepted\n";
        return false;
    }
    return true;
}

bool TestVerifiedScriptThreadsIdentity()
{
    constexpr std::uintptr_t moduleBase = 0x00007FF600000000ULL;
    const auto validation = GTA_Target_Semantic_Validator::Validate(
        ScriptThreadsDefinition(), VerifiedScriptThreadsEvidence(moduleBase),
        GTA_Target_Semantic_Validator::SupportedFingerprint, moduleBase, 0);

    if (!validation.applicable || !validation.passed) {
        std::cerr << "Verified ScriptThreads semantic identity was rejected: "
                  << validation.detail << '\n';
        return false;
    }
    return true;
}

bool TestWrongDispatchTableRva()
{
    constexpr std::uintptr_t moduleBase = 0x00007FF600000000ULL;
    auto evidence = VerifiedScriptThreadsEvidence(moduleBase);
    evidence.objectDominantFirstQwordAddress += 0x10;

    const auto validation = GTA_Target_Semantic_Validator::Validate(
        ScriptThreadsDefinition(), evidence,
        GTA_Target_Semantic_Validator::SupportedFingerprint, moduleBase, 0);

    if (!validation.applicable || validation.passed) {
        std::cerr << "Wrong ScriptThreads dispatch-table identity was accepted\n";
        return false;
    }
    return true;
}

bool TestWrongScriptThreadsFingerprint()
{
    constexpr std::uintptr_t moduleBase = 0x00007FF600000000ULL;
    const auto validation = GTA_Target_Semantic_Validator::Validate(
        ScriptThreadsDefinition(), VerifiedScriptThreadsEvidence(moduleBase),
        GTA_Target_Semantic_Validator::SupportedFingerprint + 1, moduleBase, 0);

    if (!validation.applicable || validation.passed) {
        std::cerr << "Unregistered GTA fingerprint was accepted for ScriptThreads semantic identity\n";
        return false;
    }
    return true;
}

bool TestVerifiedRunScriptThreadsIdentity()
{
    constexpr std::uintptr_t moduleBase = 0x00007FF600000000ULL;
    const auto candidate = moduleBase + GTA_Target_Semantic_Validator::RunScriptThreadsEntryRva;
    const auto validation = GTA_Target_Semantic_Validator::Validate(
        RunScriptThreadsDefinition(), VerifiedRunScriptThreadsEvidence(),
        GTA_Target_Semantic_Validator::SupportedFingerprint, moduleBase, candidate);

    if (!validation.applicable || !validation.passed) {
        std::cerr << "Verified RunScriptThreads callable entry was rejected: "
                  << validation.detail << '\n';
        return false;
    }
    return true;
}

bool TestWrongRunScriptThreadsRva()
{
    constexpr std::uintptr_t moduleBase = 0x00007FF600000000ULL;
    const auto candidate = moduleBase + GTA_Target_Semantic_Validator::RunScriptThreadsEntryRva + 1;
    const auto validation = GTA_Target_Semantic_Validator::Validate(
        RunScriptThreadsDefinition(), VerifiedRunScriptThreadsEvidence(),
        GTA_Target_Semantic_Validator::SupportedFingerprint, moduleBase, candidate);

    if (!validation.applicable || validation.passed) {
        std::cerr << "Wrong RunScriptThreads callable-entry RVA was accepted\n";
        return false;
    }
    return true;
}

bool TestWrongRunScriptThreadsBytes()
{
    constexpr std::uintptr_t moduleBase = 0x00007FF600000000ULL;
    auto evidence = VerifiedRunScriptThreadsEvidence();
    evidence.samplePreview = "90 90 90 90";
    const auto candidate = moduleBase + GTA_Target_Semantic_Validator::RunScriptThreadsEntryRva;
    const auto validation = GTA_Target_Semantic_Validator::Validate(
        RunScriptThreadsDefinition(), evidence,
        GTA_Target_Semantic_Validator::SupportedFingerprint, moduleBase, candidate);

    if (!validation.applicable || validation.passed) {
        std::cerr << "Wrong RunScriptThreads callable-entry bytes were accepted\n";
        return false;
    }
    return true;
}

bool TestVerifiedNativeBootstrapIdentity()
{
    constexpr std::uintptr_t moduleBase = 0x00007FF600000000ULL;
    const auto candidate = moduleBase + GTA_Target_Semantic_Validator::InitNativeTablesEntryRva;
    const auto validation = GTA_Target_Semantic_Validator::Validate(
        InitNativeTablesDefinition(), VerifiedNativeBootstrapEvidence(),
        GTA_Target_Semantic_Validator::SupportedFingerprint, moduleBase, candidate);

    if (!validation.applicable || !validation.passed) {
        std::cerr << "Verified InitNativeTables callable entry was rejected: "
                  << validation.detail << '\n';
        return false;
    }
    return true;
}

bool TestWrongNativeBootstrapRva()
{
    constexpr std::uintptr_t moduleBase = 0x00007FF600000000ULL;
    const auto candidate = moduleBase + GTA_Target_Semantic_Validator::InitNativeTablesEntryRva + 1;
    const auto validation = GTA_Target_Semantic_Validator::Validate(
        InitNativeTablesDefinition(), VerifiedNativeBootstrapEvidence(),
        GTA_Target_Semantic_Validator::SupportedFingerprint, moduleBase, candidate);

    if (!validation.applicable || validation.passed) {
        std::cerr << "Wrong InitNativeTables callable-entry RVA was accepted\n";
        return false;
    }
    return true;
}

bool TestWrongNativeBootstrapFingerprint()
{
    constexpr std::uintptr_t moduleBase = 0x00007FF600000000ULL;
    const auto candidate = moduleBase + GTA_Target_Semantic_Validator::InitNativeTablesEntryRva;
    const auto validation = GTA_Target_Semantic_Validator::Validate(
        InitNativeTablesDefinition(), VerifiedNativeBootstrapEvidence(),
        GTA_Target_Semantic_Validator::SupportedFingerprint + 1, moduleBase, candidate);

    if (!validation.applicable || validation.passed) {
        std::cerr << "Unregistered GTA fingerprint was accepted for InitNativeTables semantic identity\n";
        return false;
    }
    return true;
}

bool TestRuntimeReadyRequiresProvenCoreTargets()
{
    GTA_Build_Registry registry;
    constexpr auto fingerprint = GTA_Target_Semantic_Validator::SupportedFingerprint;

    std::vector<GTA_Runtime_Target_Status> statuses{
        {GTA_Runtime_Target_Id::ScriptThreads, GTA_Runtime_Target_State::Validated, true, "ScriptThreads", "validated"},
        {GTA_Runtime_Target_Id::RunScriptThreads, GTA_Runtime_Target_State::Validated, true, "RunScriptThreads", "validated"},
        {GTA_Runtime_Target_Id::InitNativeTables, GTA_Runtime_Target_State::Validated, true, "InitNativeTables", "validated"},
        {GTA_Runtime_Target_Id::GameState, GTA_Runtime_Target_State::Unknown, false, "GameState", "optional"},
        {GTA_Runtime_Target_Id::FrameCount, GTA_Runtime_Target_State::Unknown, false, "FrameCount", "optional"},
        {GTA_Runtime_Target_Id::ScriptGlobals, GTA_Runtime_Target_State::Validated, false, "ScriptGlobals", "optional validated"},
        {GTA_Runtime_Target_Id::ProgramTable, GTA_Runtime_Target_State::Located, false, "ProgramTable", "optional located"}
    };

    if (!registry.ApplyTargetStatuses(fingerprint, statuses)) {
        std::cerr << "Failed to apply runtime readiness statuses\n";
        return false;
    }

    const auto profile = registry.Find(fingerprint);
    if (!profile || profile->RequiredTargetCount() != 3 ||
        profile->ValidatedRequiredTargetCount() != 3 ||
        !profile->AllRequiredTargetsValidated() ||
        profile->verification != GTA_Build_Verification_State::Supported) {
        std::cerr << "Proven core targets did not produce a supported runtime profile\n";
        return false;
    }

    return true;
}

bool TestRuntimeNotReadyWithoutBridgeEntry()
{
    GTA_Build_Registry registry;
    constexpr auto fingerprint = GTA_Target_Semantic_Validator::SupportedFingerprint;

    std::vector<GTA_Runtime_Target_Status> statuses{
        {GTA_Runtime_Target_Id::ScriptThreads, GTA_Runtime_Target_State::Validated, true, "ScriptThreads", "validated"},
        {GTA_Runtime_Target_Id::RunScriptThreads, GTA_Runtime_Target_State::Located, true, "RunScriptThreads", "located"},
        {GTA_Runtime_Target_Id::InitNativeTables, GTA_Runtime_Target_State::Validated, true, "InitNativeTables", "validated"}
    };

    if (!registry.ApplyTargetStatuses(fingerprint, statuses)) {
        std::cerr << "Failed to apply incomplete runtime readiness statuses\n";
        return false;
    }

    const auto profile = registry.Find(fingerprint);
    if (!profile || profile->AllRequiredTargetsValidated() ||
        profile->verification != GTA_Build_Verification_State::PartiallyVerified) {
        std::cerr << "Runtime profile became ready without validated RunScriptThreads\n";
        return false;
    }

    return true;
}
}

int main()
{
    if (!TestVerifiedScriptGlobalsIdentity() ||
        !TestWrongScriptGlobalsRva() ||
        !TestVerifiedScriptThreadsIdentity() ||
        !TestWrongDispatchTableRva() ||
        !TestWrongScriptThreadsFingerprint() ||
        !TestVerifiedRunScriptThreadsIdentity() ||
        !TestWrongRunScriptThreadsRva() ||
        !TestWrongRunScriptThreadsBytes() ||
        !TestVerifiedNativeBootstrapIdentity() ||
        !TestWrongNativeBootstrapRva() ||
        !TestWrongNativeBootstrapFingerprint() ||
        !TestRuntimeReadyRequiresProvenCoreTargets() ||
        !TestRuntimeNotReadyWithoutBridgeEntry()) {
        return 1;
    }

    std::cout << "GTA target semantic validator tests passed\n";
    return 0;
}
