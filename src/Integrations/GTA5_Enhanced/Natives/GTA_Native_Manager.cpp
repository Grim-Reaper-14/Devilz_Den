#include "GTA_Native_Manager.hpp"

#include <Windows.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>

namespace Devilz::Integrations::GTA5_Enhanced
{
namespace
{
constexpr std::array<std::uint8_t, 12> ProgramTablePattern{
    0x48, 0xC7, 0x84, 0xC8, 0xD8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
constexpr std::size_t ScriptProgramCount = 176;

bool IsExecutableProtection(DWORD protection) noexcept
{
    const auto base = protection & 0xFFu;
    return base == PAGE_EXECUTE ||
           base == PAGE_EXECUTE_READ ||
           base == PAGE_EXECUTE_READWRITE ||
           base == PAGE_EXECUTE_WRITECOPY;
}

bool IsReadableProtection(DWORD protection) noexcept
{
    const auto base = protection & 0xFFu;
    return base == PAGE_READONLY ||
           base == PAGE_READWRITE ||
           base == PAGE_WRITECOPY ||
           base == PAGE_EXECUTE_READ ||
           base == PAGE_EXECUTE_READWRITE ||
           base == PAGE_EXECUTE_WRITECOPY;
}

bool IsReadableRange(std::uintptr_t address, std::size_t size) noexcept
{
    if (address == 0 || size == 0)
        return false;

    MEMORY_BASIC_INFORMATION memory{};
    if (::VirtualQuery(reinterpret_cast<const void*>(address), &memory, sizeof(memory)) == 0)
        return false;

    if (memory.State != MEM_COMMIT || (memory.Protect & (PAGE_GUARD | PAGE_NOACCESS)) != 0 ||
        !IsReadableProtection(memory.Protect)) {
        return false;
    }

    const auto start = reinterpret_cast<std::uintptr_t>(memory.BaseAddress);
    if (start > (std::numeric_limits<std::uintptr_t>::max)() - memory.RegionSize)
        return false;
    const auto end = start + memory.RegionSize;
    return address >= start && address <= end && size <= end - address;
}

std::uintptr_t FindUniqueProgramTablePattern(
    std::uintptr_t moduleBase,
    std::size_t moduleSize) noexcept
{
    if (moduleBase == 0 || moduleSize < ProgramTablePattern.size() ||
        moduleBase > (std::numeric_limits<std::uintptr_t>::max)() - moduleSize) {
        return 0;
    }

    const auto moduleEnd = moduleBase + moduleSize;
    std::uintptr_t matchAddress = 0;
    std::size_t matches = 0;
    std::uintptr_t cursor = moduleBase;

    while (cursor < moduleEnd) {
        MEMORY_BASIC_INFORMATION memory{};
        if (::VirtualQuery(reinterpret_cast<const void*>(cursor), &memory, sizeof(memory)) == 0)
            return 0;

        const auto regionBase = reinterpret_cast<std::uintptr_t>(memory.BaseAddress);
        if (regionBase > (std::numeric_limits<std::uintptr_t>::max)() - memory.RegionSize)
            return 0;
        const auto regionEndRaw = regionBase + memory.RegionSize;
        const auto regionStart = (std::max)(cursor, regionBase);
        const auto regionEnd = (std::min)(moduleEnd, regionEndRaw);

        if (memory.State == MEM_COMMIT && memory.Type == MEM_IMAGE &&
            (memory.Protect & (PAGE_GUARD | PAGE_NOACCESS)) == 0 &&
            IsExecutableProtection(memory.Protect) && regionEnd > regionStart &&
            regionEnd - regionStart >= ProgramTablePattern.size()) {
            const auto* bytes = reinterpret_cast<const std::uint8_t*>(regionStart);
            const std::size_t regionSize = regionEnd - regionStart;
            for (std::size_t offset = 0;
                 offset + ProgramTablePattern.size() <= regionSize;
                 ++offset) {
                if (std::memcmp(bytes + offset,
                                ProgramTablePattern.data(),
                                ProgramTablePattern.size()) != 0) {
                    continue;
                }

                ++matches;
                matchAddress = regionStart + offset;
                if (matches > 1)
                    return 0;
            }
        }

        if (regionEnd <= cursor)
            return 0;
        cursor = regionEnd;
    }

    return matches == 1 ? matchAddress : 0;
}

std::uintptr_t ResolveProgramTableAddress(
    std::uintptr_t moduleBase,
    std::size_t moduleSize) noexcept
{
    const auto match = FindUniqueProgramTablePattern(moduleBase, moduleSize);
    if (match == 0 || match > (std::numeric_limits<std::uintptr_t>::max)() - 0x1A)
        return 0;

    // Same resolve chain as the verified ProgramTable target:
    // match + 0x13 -> RIP-relative displacement at +3 -> resolved + 0xD8.
    const auto instruction = match + 0x13;
    const auto displacementAddress = instruction + 3;
    if (!IsReadableRange(displacementAddress, sizeof(std::int32_t)))
        return 0;

    std::int32_t displacement = 0;
    std::memcpy(&displacement,
                reinterpret_cast<const void*>(displacementAddress),
                sizeof(displacement));

    const auto instructionEnd = static_cast<std::intptr_t>(displacementAddress) +
                                static_cast<std::intptr_t>(sizeof(displacement));
    const auto resolvedSigned = instructionEnd + static_cast<std::intptr_t>(displacement);
    if (resolvedSigned <= 0 ||
        resolvedSigned > (std::numeric_limits<std::intptr_t>::max)() - 0xD8) {
        return 0;
    }

    const auto programTableAddress = static_cast<std::uintptr_t>(resolvedSigned + 0xD8);
    if (!IsReadableRange(programTableAddress, ScriptProgramCount * sizeof(void*)))
        return 0;

    return programTableAddress;
}
}

GTA_Native_Manager_Status GTA_Native_Manager::Initialize(
    std::uintptr_t initNativeTablesAddress,
    std::uintptr_t moduleBase,
    std::size_t moduleSize,
    std::uint64_t fingerprint)
{
    Reset();

    GTA_Native_Manager_Status status{};

    if (initNativeTablesAddress == 0 || moduleBase == 0 || moduleSize == 0 || fingerprint == 0) {
        status.detail = "Native bootstrap prerequisites are incomplete";
        return status;
    }

    if (!IsExecutableImageAddress(initNativeTablesAddress, moduleBase, moduleSize)) {
        status.detail = "InitNativeTables is not an executable GTA image address";
        return status;
    }

    constexpr std::size_t namedNativeCount = GTA_Native_Registry::NamedIds.size();
    constexpr std::size_t requestedHandlerCount = BootstrapProbeHashes.size() + namedNativeCount;
    std::array<GTA_Native_Hash, requestedHandlerCount> requestedHashes{};
    std::copy(BootstrapProbeHashes.begin(), BootstrapProbeHashes.end(), requestedHashes.begin());

    for (std::size_t i = 0; i < GTA_Native_Registry::NamedIds.size(); ++i) {
        const auto definition = GTA_Native_Registry::Find(fingerprint, GTA_Native_Registry::NamedIds[i]);
        if (!definition) {
            status.detail = "No complete native registry is available for this GTA build fingerprint";
            return status;
        }
        requestedHashes[BootstrapProbeHashes.size() + i] = definition->enhancedHash;
    }

    status.requestedHandlers = requestedHashes.size();

    std::array<GTA_Native_Handler, requestedHandlerCount> entries{};
    for (std::size_t i = 0; i < requestedHashes.size(); ++i) {
        entries[i] = reinterpret_cast<GTA_Native_Handler>(
            static_cast<std::uintptr_t>(requestedHashes[i]));
    }

    GTA_Native_Program_Bootstrap program{};
    program.nativeCount = static_cast<std::uint32_t>(entries.size());
    program.nativeEntrypoints = entries.data();

    const auto initialize = reinterpret_cast<GTA_Init_Native_Tables>(initNativeTablesAddress);
    initialize(&program);

    for (std::size_t i = 0; i < entries.size(); ++i) {
        const auto handlerAddress = reinterpret_cast<std::uintptr_t>(entries[i]);
        if (handlerAddress == 0 ||
            !IsExecutableImageAddress(handlerAddress, moduleBase, moduleSize)) {
            status.cachedHandlers = m_handlers.size();
            status.detail = "Native bootstrap returned a handler outside executable GTA image memory";
            Reset();
            return status;
        }

        m_handlers.insert_or_assign(requestedHashes[i], entries[i]);
    }

    m_ready = m_handlers.size() == requestedHashes.size();
    std::uintptr_t programTableAddress = 0;
    if (m_ready) {
        m_fingerprint = fingerprint;
        programTableAddress = ResolveProgramTableAddress(moduleBase, moduleSize);
        ConfigureVehicleLSCRestrictions(programTableAddress);
    }

    status.ready = m_ready;
    status.cachedHandlers = m_handlers.size();
    status.detail = m_ready
        ? (programTableAddress != 0
               ? "Enhanced native bootstrap resolved and validated probe/gameplay handlers; LSC script program table ready"
               : "Enhanced native bootstrap resolved and validated probe/gameplay handlers; LSC script program table unavailable")
        : "Enhanced native bootstrap did not populate the complete handler set";
    return status;
}

void GTA_Native_Manager::Reset() noexcept
{
    ResetStatsExtension();
    GTA_Packed_Stats_State::Instance().Reset();
    ResetVehicleLSCRestrictions();

    if (m_ready) {
        SetSelfSpecialAbilities(false);
        SetSelfNoIdleKick(false);
        TickSelfUtilityExtension(*this);
    }
    ResetSelfUtilityExtension();

    m_ready = false;
    m_fingerprint = 0;
    m_handlers.clear();
}

GTA_Native_Handler GTA_Native_Manager::Find(GTA_Native_Hash hash) const noexcept
{
    const auto it = m_handlers.find(hash);
    return it == m_handlers.end() ? nullptr : it->second;
}

GTA_Native_Handler GTA_Native_Manager::Find(GTA_Native_Id id) const noexcept
{
    const auto definition = GTA_Native_Registry::Find(m_fingerprint, id);
    return definition ? Find(definition->enhancedHash) : nullptr;
}

bool GTA_Native_Manager::IsExecutableImageAddress(
    std::uintptr_t address,
    std::uintptr_t moduleBase,
    std::size_t moduleSize) noexcept
{
    if (address < moduleBase ||
        moduleBase > (std::numeric_limits<std::uintptr_t>::max)() - moduleSize ||
        address >= moduleBase + moduleSize) {
        return false;
    }

    MEMORY_BASIC_INFORMATION memory{};
    if (::VirtualQuery(reinterpret_cast<const void*>(address), &memory, sizeof(memory)) == 0)
        return false;

    if (memory.State != MEM_COMMIT || memory.Type != MEM_IMAGE)
        return false;

    if ((memory.Protect & PAGE_GUARD) != 0 || (memory.Protect & PAGE_NOACCESS) != 0)
        return false;

    return IsExecutableProtection(memory.Protect);
}
}
