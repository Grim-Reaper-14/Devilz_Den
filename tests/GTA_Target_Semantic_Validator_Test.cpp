#include "Integrations/GTA5_Enhanced/Memory/GTA_Target_Semantic_Validator.hpp"

#include <cstdint>
#include <iostream>

namespace
{
using namespace Devilz::Integrations::GTA5_Enhanced;

GTA_Target_Definition ScriptThreadsDefinition()
{
    GTA_Target_Definition definition{};
    definition.id = GTA_Runtime_Target_Id::ScriptThreads;
    definition.name = "ScriptThreads";
    definition.candidateKind = GTA_Target_Candidate_Kind::PointerStorage;
    return definition;
}

GTA_Target_Evidence VerifiedEvidence(std::uintptr_t moduleBase)
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

bool TestVerifiedIdentity()
{
    constexpr std::uintptr_t moduleBase = 0x00007FF600000000ULL;
    const auto validation = GTA_Target_Semantic_Validator::Validate(
        ScriptThreadsDefinition(),
        VerifiedEvidence(moduleBase),
        GTA_Target_Semantic_Validator::SupportedFingerprint,
        moduleBase);

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
    auto evidence = VerifiedEvidence(moduleBase);
    evidence.objectDominantFirstQwordAddress += 0x10;

    const auto validation = GTA_Target_Semantic_Validator::Validate(
        ScriptThreadsDefinition(),
        evidence,
        GTA_Target_Semantic_Validator::SupportedFingerprint,
        moduleBase);

    if (!validation.applicable || validation.passed) {
        std::cerr << "Wrong ScriptThreads dispatch-table identity was accepted\n";
        return false;
    }

    return true;
}

bool TestWrongFingerprint()
{
    constexpr std::uintptr_t moduleBase = 0x00007FF600000000ULL;
    const auto validation = GTA_Target_Semantic_Validator::Validate(
        ScriptThreadsDefinition(),
        VerifiedEvidence(moduleBase),
        GTA_Target_Semantic_Validator::SupportedFingerprint + 1,
        moduleBase);

    if (!validation.applicable || validation.passed) {
        std::cerr << "Unregistered GTA fingerprint was accepted for ScriptThreads semantic identity\n";
        return false;
    }

    return true;
}
}

int main()
{
    if (!TestVerifiedIdentity() ||
        !TestWrongDispatchTableRva() ||
        !TestWrongFingerprint()) {
        return 1;
    }

    std::cout << "GTA target semantic validator tests passed\n";
    return 0;
}
