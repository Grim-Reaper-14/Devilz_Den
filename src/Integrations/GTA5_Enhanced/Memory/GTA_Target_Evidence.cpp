#include "GTA_Target_Evidence.hpp"

#include "Backend/Process/Process_Memory_Reader.hpp"

#include <Windows.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <iomanip>
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
    DWORD type = 0;
    std::uintptr_t base = 0;
    std::size_t size = 0;
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
    info.type = region.Type;
    info.base = reinterpret_cast<std::uintptr_t>(region.BaseAddress);
    info.size = region.RegionSize;

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

std::string HexValue(std::uint64_t value)
{
    std::ostringstream stream;
    stream << "0x" << std::uppercase << std::hex << value;
    return stream.str();
}

std::string HexBytes(const std::byte* data, std::size_t size)
{
    std::ostringstream stream;
    stream << std::uppercase << std::hex << std::setfill('0');
    for (std::size_t i = 0; i < size; ++i) {
        if (i != 0)
            stream << ' ';
        stream << std::setw(2) << static_cast<unsigned>(std::to_integer<std::uint8_t>(data[i]));
    }
    return stream.str();
}

std::string QwordPreview(
    const std::byte* data,
    std::size_t size,
    std::size_t& nonZero,
    std::size_t& readablePointers)
{
    std::ostringstream stream;
    const auto count = size / sizeof(std::uint64_t);
    for (std::size_t i = 0; i < count; ++i) {
        std::uint64_t value = 0;
        std::memcpy(&value, data + (i * sizeof(value)), sizeof(value));
        if (value != 0)
            ++nonZero;
        if (QueryRegion(static_cast<std::uintptr_t>(value)).readable)
            ++readablePointers;
        if (i != 0)
            stream << ',';
        stream << HexValue(value);
    }
    return stream.str();
}

std::string PointerTablePreview(
    const std::byte* data,
    std::size_t size,
    std::size_t& readablePointers,
    std::size_t& executableImagePointers)
{
    std::ostringstream stream;
    const auto count = size / sizeof(std::uintptr_t);

    for (std::size_t i = 0; i < count; ++i) {
        std::uintptr_t value = 0;
        std::memcpy(&value, data + (i * sizeof(value)), sizeof(value));

        const auto region = QueryRegion(value);
        if (region.readable)
            ++readablePointers;
        if (region.type == MEM_IMAGE && region.executable)
            ++executableImagePointers;

        if (i != 0)
            stream << ',';
        stream << HexValue(value);
    }

    return stream.str();
}

void CaptureRegion(const Region_Info& region, GTA_Target_Evidence& evidence)
{
    evidence.candidateCommitted = region.committed;
    evidence.candidateReadable = region.readable;
    evidence.candidateWritable = region.writable;
    evidence.candidateExecutable = region.executable;
    evidence.candidateProtection = region.protection;
    evidence.candidateType = region.type;
    evidence.candidateRegionBase = region.base;
    evidence.candidateRegionSize = region.size;
}

void ProbeObjectSlots(
    Backend::Process_Memory_Reader& reader,
    const std::vector<std::byte>& sample,
    GTA_Target_Evidence& evidence)
{
    constexpr std::size_t maxSlots = 8;
    const auto availableSlots = sample.size() / sizeof(std::uintptr_t);
    const auto slotCount = std::min(maxSlots, availableSlots);

    std::array<std::uintptr_t, maxSlots> decodedTargets{};
    std::size_t decodedTargetCount = 0;

    std::ostringstream preview;
    for (std::size_t i = 0; i < slotCount; ++i) {
        std::uintptr_t objectAddress = 0;
        std::memcpy(&objectAddress,
                    sample.data() + (i * sizeof(std::uintptr_t)),
                    sizeof(objectAddress));

        ++evidence.objectSlotsSampled;
        const auto objectRegion = QueryRegion(objectAddress);
        if (objectRegion.readable)
            ++evidence.readableObjects;

        std::uintptr_t firstQword = 0;
        bool decoded = false;
        Region_Info firstQwordRegion{};

        if (objectAddress != 0 && objectRegion.readable) {
            auto first = reader.Read(objectAddress, sizeof(std::uintptr_t));
            if (first) {
                std::memcpy(&firstQword, first.Value().data(), sizeof(firstQword));
                decoded = true;
                ++evidence.objectFirstQwordsDecoded;

                firstQwordRegion = QueryRegion(firstQword);
                if (firstQwordRegion.type == MEM_IMAGE)
                    ++evidence.objectFirstQwordImagePointers;
                if (firstQwordRegion.type == MEM_IMAGE && firstQwordRegion.executable)
                    ++evidence.objectFirstQwordExecutableImagePointers;

                if (firstQword != 0 && decodedTargetCount < decodedTargets.size())
                    decodedTargets[decodedTargetCount++] = firstQword;
            }
        }

        if (i != 0)
            preview << ';';
        preview << i << ':' << HexValue(objectAddress);
        if (decoded)
            preview << "->" << HexValue(firstQword)
                    << "[T=" << HexValue(firstQwordRegion.type)
                    << ",P=" << HexValue(firstQwordRegion.protection) << ']';
        else
            preview << "->unreadable";
    }

    std::uintptr_t dominantAddress = 0;
    std::size_t dominantCount = 0;
    for (std::size_t i = 0; i < decodedTargetCount; ++i) {
        std::size_t count = 0;
        for (std::size_t j = 0; j < decodedTargetCount; ++j) {
            if (decodedTargets[j] == decodedTargets[i])
                ++count;
        }

        if (count > dominantCount) {
            dominantAddress = decodedTargets[i];
            dominantCount = count;
        }
    }

    evidence.objectDominantFirstQwordAddress = dominantAddress;
    evidence.objectDominantFirstQwordCount = dominantCount;

    const auto dominantRegion = QueryRegion(dominantAddress);
    if (dominantAddress != 0 && dominantRegion.readable) {
        auto dominantSample = reader.Read(dominantAddress, 64);
        if (dominantSample) {
            evidence.objectDominantFirstQwordSampleRead = true;

            const auto bytePreviewSize = std::min<std::size_t>(16, dominantSample.Value().size());
            evidence.objectDominantFirstQwordBytes = HexBytes(
                dominantSample.Value().data(),
                bytePreviewSize);

            evidence.objectDominantFirstQwordQwordPreview = PointerTablePreview(
                dominantSample.Value().data(),
                dominantSample.Value().size(),
                evidence.objectDominantFirstQwordReadablePointers,
                evidence.objectDominantFirstQwordExecutableImagePointers);
        }
    }

    evidence.objectSamplePreview = preview.str();
}
}

GTA_Target_Evidence GTA_Target_Evidence_Probe::Probe(
    std::uint32_t pid,
    GTA_Target_Candidate_Kind kind,
    std::uintptr_t address)
{
    GTA_Target_Evidence evidence{};
    const auto region = QueryRegion(address);
    CaptureRegion(region, evidence);

    Backend::Process_Memory_Reader reader(pid);

    if (address != 0 && pid != 0 && region.readable) {
        if (kind == GTA_Target_Candidate_Kind::CodeSite) {
            auto bytes = reader.Read(address, 16);
            if (bytes) {
                evidence.sampleRead = true;
                evidence.samplePreview = HexBytes(bytes.Value().data(), bytes.Value().size());
            }
        } else if (kind == GTA_Target_Candidate_Kind::DirectData) {
            auto bytes = reader.Read(address, 64);
            if (bytes) {
                evidence.sampleRead = true;
                evidence.samplePreview = QwordPreview(
                    bytes.Value().data(),
                    bytes.Value().size(),
                    evidence.sampleNonZeroQwords,
                    evidence.sampleReadablePointers);
            }
        }
    }

    if (kind == GTA_Target_Candidate_Kind::PointerStorage && address != 0 && pid != 0) {
        auto bytes = reader.Read(address, sizeof(std::uintptr_t));
        if (bytes) {
            std::uintptr_t pointee = 0;
            std::memcpy(&pointee, bytes.Value().data(), sizeof(pointee));
            evidence.pointerDecoded = true;
            evidence.pointeeAddress = pointee;

            const auto pointeeRegion = QueryRegion(pointee);
            evidence.pointeeCommitted = pointeeRegion.committed;
            evidence.pointeeReadable = pointeeRegion.readable;
            evidence.pointeeWritable = pointeeRegion.writable;
            evidence.pointeeExecutable = pointeeRegion.executable;
            evidence.pointeeProtection = pointeeRegion.protection;
            evidence.pointeeType = pointeeRegion.type;
            evidence.pointeeRegionBase = pointeeRegion.base;
            evidence.pointeeRegionSize = pointeeRegion.size;

            if (pointee != 0 && pointeeRegion.readable) {
                auto sample = reader.Read(pointee, 64);
                if (sample) {
                    evidence.pointeeSampleRead = true;
                    evidence.pointeeSamplePreview = QwordPreview(
                        sample.Value().data(),
                        sample.Value().size(),
                        evidence.pointeeSampleNonZeroQwords,
                        evidence.pointeeSampleReadablePointers);
                    ProbeObjectSlots(reader, sample.Value(), evidence);
                }
            }
        }
    }

    std::ostringstream summary;
    summary << "Kind=" << KindName(kind)
            << " | CandidateCommitted=" << (evidence.candidateCommitted ? "yes" : "no")
            << " | Readable=" << (evidence.candidateReadable ? "yes" : "no")
            << " | Writable=" << (evidence.candidateWritable ? "yes" : "no")
            << " | Executable=" << (evidence.candidateExecutable ? "yes" : "no")
            << " | Protect=" << HexValue(evidence.candidateProtection)
            << " | Type=" << HexValue(evidence.candidateType)
            << " | Region=" << HexValue(evidence.candidateRegionBase)
            << "+" << HexValue(evidence.candidateRegionSize);

    if (evidence.sampleRead) {
        if (kind == GTA_Target_Candidate_Kind::CodeSite) {
            summary << " | Bytes=" << evidence.samplePreview;
        } else {
            summary << " | Qwords=" << evidence.samplePreview
                    << " | NonZero=" << evidence.sampleNonZeroQwords
                    << " | ReadablePtrLike=" << evidence.sampleReadablePointers;
        }
    }

    if (kind == GTA_Target_Candidate_Kind::PointerStorage) {
        summary << " | PointerDecoded=" << (evidence.pointerDecoded ? "yes" : "no");
        if (evidence.pointerDecoded) {
            summary << " | Pointee=" << HexValue(evidence.pointeeAddress)
                    << " | PointeeCommitted=" << (evidence.pointeeCommitted ? "yes" : "no")
                    << " | PointeeReadable=" << (evidence.pointeeReadable ? "yes" : "no")
                    << " | PointeeWritable=" << (evidence.pointeeWritable ? "yes" : "no")
                    << " | PointeeExecutable=" << (evidence.pointeeExecutable ? "yes" : "no")
                    << " | PointeeProtect=" << HexValue(evidence.pointeeProtection)
                    << " | PointeeType=" << HexValue(evidence.pointeeType)
                    << " | PointeeRegion=" << HexValue(evidence.pointeeRegionBase)
                    << "+" << HexValue(evidence.pointeeRegionSize);

            if (evidence.pointeeSampleRead) {
                summary << " | PointeeQwords=" << evidence.pointeeSamplePreview
                        << " | PointeeNonZero=" << evidence.pointeeSampleNonZeroQwords
                        << " | PointeeReadablePtrLike=" << evidence.pointeeSampleReadablePointers
                        << " | ObjectSlots=" << evidence.objectSlotsSampled
                        << " | ReadableObjects=" << evidence.readableObjects
                        << " | FirstQwordsDecoded=" << evidence.objectFirstQwordsDecoded
                        << " | FirstQwordImagePtrs=" << evidence.objectFirstQwordImagePointers
                        << " | FirstQwordExecImagePtrs=" << evidence.objectFirstQwordExecutableImagePointers
                        << " | Objects=" << evidence.objectSamplePreview;

                if (evidence.objectDominantFirstQwordAddress != 0) {
                    summary << " | DominantFirstQword=" << HexValue(evidence.objectDominantFirstQwordAddress)
                            << " | DominantCount=" << evidence.objectDominantFirstQwordCount
                            << "/" << evidence.objectFirstQwordsDecoded;

                    if (evidence.objectDominantFirstQwordSampleRead) {
                        summary << " | DominantBytes=" << evidence.objectDominantFirstQwordBytes
                                << " | DominantQwords=" << evidence.objectDominantFirstQwordQwordPreview
                                << " | DominantReadablePtrs=" << evidence.objectDominantFirstQwordReadablePointers
                                << " | DominantExecImagePtrs="
                                << evidence.objectDominantFirstQwordExecutableImagePointers;
                    }
                }
            }
        }
    }

    evidence.summary = summary.str();
    return evidence;
}
}
