#include "GTA_Target_Evidence.hpp"

#include "Backend/Process/Process_Memory_Reader.hpp"

#include <Windows.h>

#include <cstddef>
#include <cstring>
#include <sstream>

namespace Devilz::Integrations::GTA5_Enhanced
{
namespace
{
struct Region_Info
{
    bool committed = false;
    bool readable = false;
    bool writable = false;
    bool executable = false;
    DWORD protection = 0;
};

bool HasAny(DWORD value, DWORD mask) noexcept
{
    return (value & mask) != 0;
}

Region_Info QueryRegion(std::uintptr_t address) noexcept
{
    Region_Info info{};
    if (address == 0)
        return info;

    MEMORY_BASIC_INFORMATION region{};
    if (::VirtualQuery(reinterpret_cast<LPCVOID>(address), &region, sizeof(region)) == 0)
        return info;

    info.committed = region.State == MEM_COMMIT;
    info.protection = region.Protect;

    if (!info.committed || HasAny(region.Protect, PAGE_GUARD | PAGE_NOACCESS))
        return info;

    info.readable = HasAny(region.Protect,
        PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY |
        PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY);

    info.writable = HasAny(region.Protect,
        PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY);

    info.executable = HasAny(region.Protect,
        PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY);

    return info;
}

const char* KindName(GTA_Target_Candidate_Kind kind) noexcept
{
    switch (kind) {
    case GTA_Target_Candidate_Kind::DirectData: return "DirectData";
    case GTA_Target_Candidate_Kind::PointerStorage: return "PointerStorage";
    case GTA_Target_Candidate_Kind::CodeSite: return "CodeSite";
    default: return "Unknown";
    }
}
}

GTA_Target_Evidence GTA_Target_Evidence_Probe::Probe(
    std::uint32_t pid,
    GTA_Target_Candidate_Kind kind,
    std::uintptr_t address)
{
    GTA_Target_Evidence evidence{};
    const auto region = QueryRegion(address);
    evidence.candidateCommitted = region.committed;
    evidence.candidateReadable = region.readable;
    evidence.candidateWritable = region.writable;
    evidence.candidateExecutable = region.executable;
    evidence.candidateProtection = region.protection;

    if (kind == GTA_Target_Candidate_Kind::PointerStorage && address != 0 && pid != 0) {
        Backend::Process_Memory_Reader reader(pid);
        auto bytes = reader.Read(address, sizeof(std::uintptr_t));
        if (bytes) {
            std::uintptr_t pointee = 0;
            std::memcpy(&pointee, bytes.Value().data(), sizeof(pointee));
            evidence.pointerDecoded = true;
            evidence.pointeeAddress = pointee;

            const auto pointeeRegion = QueryRegion(pointee);
            evidence.pointeeCommitted = pointeeRegion.committed;
            evidence.pointeeReadable = pointeeRegion.readable;
            evidence.pointeeProtection = pointeeRegion.protection;
        }
    }

    std::ostringstream summary;
    summary << "Kind=" << KindName(kind)
            << " | CandidateCommitted=" << (evidence.candidateCommitted ? "yes" : "no")
            << " | Readable=" << (evidence.candidateReadable ? "yes" : "no")
            << " | Writable=" << (evidence.candidateWritable ? "yes" : "no")
            << " | Executable=" << (evidence.candidateExecutable ? "yes" : "no");

    if (kind == GTA_Target_Candidate_Kind::PointerStorage) {
        summary << " | PointerDecoded=" << (evidence.pointerDecoded ? "yes" : "no");
        if (evidence.pointerDecoded) {
            summary << " | Pointee=0x" << std::hex << std::uppercase << evidence.pointeeAddress << std::dec
                    << " | PointeeCommitted=" << (evidence.pointeeCommitted ? "yes" : "no")
                    << " | PointeeReadable=" << (evidence.pointeeReadable ? "yes" : "no");
        }
    }

    evidence.summary = summary.str();
    return evidence;
}
}
