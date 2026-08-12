#include "GTA_Vehicle_Garage_Save.hpp"

#include "GTA_Vehicle_State.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Manager.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Registry.hpp"

#include <Windows.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <mutex>
#include <string_view>
#include <vector>

namespace Devilz::Integrations::GTA5_Enhanced
{
namespace
{
constexpr std::uint64_t SupportedFingerprint = 0x6A4F97F605B81000ULL;
constexpr std::size_t ScriptProgramCount = 176;
constexpr std::size_t ScriptCodePageSize = 0x4000;

constexpr std::array<int, 19> CanUseVehiclePattern{
    0x2D, -1, -1, -1, -1, 0x38, -1, 0x5D, -1, -1,
    -1, 0x56, -1, -1, 0x71, 0x2E, -1, -1, 0x5D
};
constexpr std::array<int, 16> BlockMenuOptionPattern{
    0x2D, -1, -1, -1, -1, 0x38, -1, 0x5D,
    -1, -1, -1, 0x5D, -1, -1, -1, 0x56
};
constexpr std::array<std::uint8_t, 4> CanUseVehiclePatch{0x72, 0x2E, 0x03, 0x01};
constexpr std::array<std::uint8_t, 4> BlockMenuOptionPatch{0x71, 0x2E, 0x01, 0x01};
constexpr std::uint32_t PatchOffset = 5;

struct GTA_Script_Program_View
{
    std::byte pad00[0x10]{};
    std::uint8_t** codeBlocks = nullptr;
    std::uint32_t hash = 0;
    std::uint32_t codeSize = 0;
    std::uint32_t argCount = 0;
    std::uint32_t localCount = 0;
    std::uint32_t globalCount = 0;
    std::uint32_t nativeCount = 0;
    void* localData = nullptr;
    void** globalData = nullptr;
    void** nativeEntrypoints = nullptr;
    std::uint32_t procCount = 0;
    std::uint32_t pad4C = 0;
    const char** procNames = nullptr;
    std::uint32_t nameHash = 0;
    std::uint32_t refCount = 0;
    const char* name = nullptr;
    const char** stringsData = nullptr;
    std::uint32_t stringsCount = 0;
    std::byte pad74[0x0C]{};
};

static_assert(sizeof(GTA_Script_Program_View) == 0x80);
static_assert(offsetof(GTA_Script_Program_View, codeBlocks) == 0x10);
static_assert(offsetof(GTA_Script_Program_View, nameHash) == 0x58);

struct ScriptPatchSite
{
    std::uint32_t pc = 0;
    std::array<std::uint8_t, 4> original{};
    bool captured = false;
};

struct LSCRestrictionRuntime
{
    std::uintptr_t programTableAddress = 0;
    GTA_Script_Program_View* program = nullptr;
    std::uint8_t** codeBlocks = nullptr;
    ScriptPatchSite canUseVehicle{};
    ScriptPatchSite blockMenuOption{};
    bool resolved = false;
    bool applied = false;
};

std::atomic_bool g_lscPrepRequested{false};
std::atomic<GTA_Vehicle_LSC_Prep_Status> g_lscPrepStatus{GTA_Vehicle_LSC_Prep_Status::Idle};
std::atomic_bool g_removeLSCRestrictions{false};
std::atomic<GTA_Vehicle_LSC_Restriction_Status> g_removeLSCRestrictionsStatus{
    GTA_Vehicle_LSC_Restriction_Status::Disabled};
std::mutex g_lscRestrictionMutex;
LSCRestrictionRuntime g_lscRuntime{};

bool IsReadableAddress(std::uintptr_t address, std::size_t size) noexcept
{
    if (address == 0 || size == 0)
        return false;

    MEMORY_BASIC_INFORMATION memory{};
    if (::VirtualQuery(reinterpret_cast<const void*>(address), &memory, sizeof(memory)) == 0)
        return false;

    if (memory.State != MEM_COMMIT || (memory.Protect & (PAGE_GUARD | PAGE_NOACCESS)) != 0)
        return false;

    const DWORD protection = memory.Protect & 0xFFU;
    const bool readable = protection == PAGE_READONLY ||
                          protection == PAGE_READWRITE ||
                          protection == PAGE_WRITECOPY ||
                          protection == PAGE_EXECUTE_READ ||
                          protection == PAGE_EXECUTE_READWRITE ||
                          protection == PAGE_EXECUTE_WRITECOPY;
    if (!readable)
        return false;

    const auto start = reinterpret_cast<std::uintptr_t>(memory.BaseAddress);
    if (start > (std::numeric_limits<std::uintptr_t>::max)() - memory.RegionSize)
        return false;
    const auto end = start + memory.RegionSize;
    return address >= start && address <= end && size <= end - address;
}

bool WriteBytes(std::uint8_t* address, const std::uint8_t* bytes, std::size_t size) noexcept
{
    if (!address || !bytes || size == 0 ||
        !IsReadableAddress(reinterpret_cast<std::uintptr_t>(address), size)) {
        return false;
    }

    DWORD oldProtection = 0;
    if (::VirtualProtect(address, size, PAGE_READWRITE, &oldProtection) == FALSE)
        return false;

    std::memcpy(address, bytes, size);

    DWORD ignored = 0;
    return ::VirtualProtect(address, size, oldProtection, &ignored) != FALSE;
}

std::uint32_t Joaat(std::string_view value) noexcept
{
    std::uint32_t hash = 0;
    for (unsigned char character : value) {
        character = static_cast<unsigned char>(std::tolower(character));
        hash += character;
        hash += hash << 10U;
        hash ^= hash >> 6U;
    }
    hash += hash << 3U;
    hash ^= hash >> 11U;
    hash += hash << 15U;
    return hash;
}

GTA_Script_Program_View* FindScriptProgram(std::uint32_t nameHash) noexcept
{
    if (!IsReadableAddress(g_lscRuntime.programTableAddress, ScriptProgramCount * sizeof(void*)))
        return nullptr;

    auto** programs = reinterpret_cast<GTA_Script_Program_View**>(g_lscRuntime.programTableAddress);
    for (std::size_t index = 0; index < ScriptProgramCount; ++index) {
        GTA_Script_Program_View* program = nullptr;
        std::memcpy(&program, programs + index, sizeof(program));
        if (!program ||
            !IsReadableAddress(reinterpret_cast<std::uintptr_t>(program), sizeof(GTA_Script_Program_View))) {
            continue;
        }

        if (program->nameHash == nameHash && program->codeBlocks && program->codeSize != 0)
            return program;
    }

    return nullptr;
}

bool SnapshotProgramCode(GTA_Script_Program_View* program, std::vector<std::uint8_t>& code) noexcept
{
    if (!program || !program->codeBlocks || program->codeSize == 0)
        return false;

    const std::size_t pageCount =
        (static_cast<std::size_t>(program->codeSize) + ScriptCodePageSize - 1) / ScriptCodePageSize;
    if (pageCount == 0 ||
        !IsReadableAddress(reinterpret_cast<std::uintptr_t>(program->codeBlocks),
                           pageCount * sizeof(std::uint8_t*))) {
        return false;
    }

    code.resize(program->codeSize);
    std::size_t copied = 0;
    for (std::size_t pageIndex = 0; pageIndex < pageCount; ++pageIndex) {
        std::uint8_t* page = nullptr;
        std::memcpy(&page, program->codeBlocks + pageIndex, sizeof(page));
        const std::size_t pageSize =
            std::min<std::size_t>(ScriptCodePageSize, static_cast<std::size_t>(program->codeSize) - copied);
        if (!page || !IsReadableAddress(reinterpret_cast<std::uintptr_t>(page), pageSize))
            return false;

        std::memcpy(code.data() + copied, page, pageSize);
        copied += pageSize;
    }

    return copied == code.size();
}

template <std::size_t N>
bool FindUniquePattern(
    const std::vector<std::uint8_t>& code,
    const std::array<int, N>& pattern,
    std::uint32_t& pc) noexcept
{
    if (code.size() < N)
        return false;

    std::size_t matches = 0;
    std::size_t matchOffset = 0;
    for (std::size_t offset = 0; offset + N <= code.size(); ++offset) {
        bool match = true;
        for (std::size_t byte = 0; byte < N; ++byte) {
            if (pattern[byte] >= 0 && code[offset + byte] != static_cast<std::uint8_t>(pattern[byte])) {
                match = false;
                break;
            }
        }

        if (!match)
            continue;

        ++matches;
        matchOffset = offset;
        if (matches > 1)
            return false;
    }

    if (matches != 1 || matchOffset > (std::numeric_limits<std::uint32_t>::max)())
        return false;

    pc = static_cast<std::uint32_t>(matchOffset);
    return true;
}

bool ReadProgramBytes(
    GTA_Script_Program_View* program,
    std::uint32_t pc,
    std::uint8_t* destination,
    std::size_t size) noexcept
{
    if (!program || !program->codeBlocks || !destination || size == 0 ||
        pc > program->codeSize || size > static_cast<std::size_t>(program->codeSize - pc)) {
        return false;
    }

    std::size_t remaining = size;
    std::size_t destinationOffset = 0;
    std::size_t cursor = pc;
    while (remaining != 0) {
        const std::size_t pageIndex = cursor / ScriptCodePageSize;
        const std::size_t pageOffset = cursor % ScriptCodePageSize;
        std::uint8_t* page = nullptr;
        std::memcpy(&page, program->codeBlocks + pageIndex, sizeof(page));
        if (!page)
            return false;

        const std::size_t chunk = std::min<std::size_t>(remaining, ScriptCodePageSize - pageOffset);
        if (!IsReadableAddress(reinterpret_cast<std::uintptr_t>(page + pageOffset), chunk))
            return false;

        std::memcpy(destination + destinationOffset, page + pageOffset, chunk);
        cursor += chunk;
        destinationOffset += chunk;
        remaining -= chunk;
    }

    return true;
}

bool WriteProgramBytes(
    GTA_Script_Program_View* program,
    std::uint32_t pc,
    const std::uint8_t* source,
    std::size_t size) noexcept
{
    if (!program || !program->codeBlocks || !source || size == 0 ||
        pc > program->codeSize || size > static_cast<std::size_t>(program->codeSize - pc)) {
        return false;
    }

    std::size_t remaining = size;
    std::size_t sourceOffset = 0;
    std::size_t cursor = pc;
    while (remaining != 0) {
        const std::size_t pageIndex = cursor / ScriptCodePageSize;
        const std::size_t pageOffset = cursor % ScriptCodePageSize;
        std::uint8_t* page = nullptr;
        std::memcpy(&page, program->codeBlocks + pageIndex, sizeof(page));
        if (!page)
            return false;

        const std::size_t chunk = std::min<std::size_t>(remaining, ScriptCodePageSize - pageOffset);
        if (!WriteBytes(page + pageOffset, source + sourceOffset, chunk))
            return false;

        cursor += chunk;
        sourceOffset += chunk;
        remaining -= chunk;
    }

    return true;
}

template <std::size_t N>
bool BytesEqual(
    GTA_Script_Program_View* program,
    std::uint32_t pc,
    const std::array<std::uint8_t, N>& expected) noexcept
{
    std::array<std::uint8_t, N> current{};
    return ReadProgramBytes(program, pc, current.data(), current.size()) && current == expected;
}

void ClearResolvedPatchState() noexcept
{
    g_lscRuntime.program = nullptr;
    g_lscRuntime.codeBlocks = nullptr;
    g_lscRuntime.canUseVehicle = {};
    g_lscRuntime.blockMenuOption = {};
    g_lscRuntime.resolved = false;
    g_lscRuntime.applied = false;
}

bool RestorePatchesLocked() noexcept
{
    if (!g_lscRuntime.resolved || !g_lscRuntime.program)
        return true;

    auto* program = g_lscRuntime.program;
    if (!IsReadableAddress(reinterpret_cast<std::uintptr_t>(program), sizeof(GTA_Script_Program_View)) ||
        program->codeBlocks != g_lscRuntime.codeBlocks) {
        ClearResolvedPatchState();
        return false;
    }

    bool success = true;
    if (g_lscRuntime.canUseVehicle.captured &&
        BytesEqual(program, g_lscRuntime.canUseVehicle.pc, CanUseVehiclePatch)) {
        success = WriteProgramBytes(
                      program,
                      g_lscRuntime.canUseVehicle.pc,
                      g_lscRuntime.canUseVehicle.original.data(),
                      g_lscRuntime.canUseVehicle.original.size()) &&
                  success;
    }

    if (g_lscRuntime.blockMenuOption.captured &&
        BytesEqual(program, g_lscRuntime.blockMenuOption.pc, BlockMenuOptionPatch)) {
        success = WriteProgramBytes(
                      program,
                      g_lscRuntime.blockMenuOption.pc,
                      g_lscRuntime.blockMenuOption.original.data(),
                      g_lscRuntime.blockMenuOption.original.size()) &&
                  success;
    }

    ClearResolvedPatchState();
    return success;
}

bool ResolvePatchSitesLocked(GTA_Script_Program_View* program) noexcept
{
    std::vector<std::uint8_t> code;
    if (!SnapshotProgramCode(program, code))
        return false;

    std::uint32_t canUseMatch = 0;
    std::uint32_t blockMenuMatch = 0;
    if (!FindUniquePattern(code, CanUseVehiclePattern, canUseMatch) ||
        !FindUniquePattern(code, BlockMenuOptionPattern, blockMenuMatch)) {
        return false;
    }

    const std::uint64_t canUsePc64 = static_cast<std::uint64_t>(canUseMatch) + PatchOffset;
    const std::uint64_t blockMenuPc64 = static_cast<std::uint64_t>(blockMenuMatch) + PatchOffset;
    if (canUsePc64 + CanUseVehiclePatch.size() > program->codeSize ||
        blockMenuPc64 + BlockMenuOptionPatch.size() > program->codeSize ||
        canUsePc64 > (std::numeric_limits<std::uint32_t>::max)() ||
        blockMenuPc64 > (std::numeric_limits<std::uint32_t>::max)()) {
        return false;
    }

    ScriptPatchSite canUse{};
    ScriptPatchSite blockMenu{};
    canUse.pc = static_cast<std::uint32_t>(canUsePc64);
    blockMenu.pc = static_cast<std::uint32_t>(blockMenuPc64);

    if (!ReadProgramBytes(program, canUse.pc, canUse.original.data(), canUse.original.size()) ||
        !ReadProgramBytes(program, blockMenu.pc, blockMenu.original.data(), blockMenu.original.size())) {
        return false;
    }

    // Both Yim signatures point at the original 0x38 opcode. Refuse to take
    // ownership of a site another patcher has already changed.
    if (canUse.original[0] != 0x38 || blockMenu.original[0] != 0x38 ||
        canUse.original == CanUseVehiclePatch || blockMenu.original == BlockMenuOptionPatch) {
        return false;
    }

    canUse.captured = true;
    blockMenu.captured = true;
    g_lscRuntime.program = program;
    g_lscRuntime.codeBlocks = program->codeBlocks;
    g_lscRuntime.canUseVehicle = canUse;
    g_lscRuntime.blockMenuOption = blockMenu;
    g_lscRuntime.resolved = true;
    g_lscRuntime.applied = false;
    return true;
}

bool ApplyPatchesLocked() noexcept
{
    if (!g_lscRuntime.resolved || !g_lscRuntime.program)
        return false;

    auto* program = g_lscRuntime.program;
    if (!WriteProgramBytes(
            program,
            g_lscRuntime.canUseVehicle.pc,
            CanUseVehiclePatch.data(),
            CanUseVehiclePatch.size())) {
        return false;
    }

    if (!WriteProgramBytes(
            program,
            g_lscRuntime.blockMenuOption.pc,
            BlockMenuOptionPatch.data(),
            BlockMenuOptionPatch.size())) {
        (void)WriteProgramBytes(
            program,
            g_lscRuntime.canUseVehicle.pc,
            g_lscRuntime.canUseVehicle.original.data(),
            g_lscRuntime.canUseVehicle.original.size());
        return false;
    }

    g_lscRuntime.applied = true;
    return true;
}

void TickLSCRestrictions(GTA_Native_Manager& natives) noexcept
{
    std::lock_guard lock(g_lscRestrictionMutex);

    if (!g_removeLSCRestrictions.load(std::memory_order_acquire)) {
        if (g_lscRuntime.resolved)
            (void)RestorePatchesLocked();
        g_removeLSCRestrictionsStatus.store(
            GTA_Vehicle_LSC_Restriction_Status::Disabled,
            std::memory_order_release);
        return;
    }

    if (natives.Fingerprint() != SupportedFingerprint || g_lscRuntime.programTableAddress == 0) {
        if (g_lscRuntime.resolved)
            (void)RestorePatchesLocked();
        g_removeLSCRestrictionsStatus.store(
            GTA_Vehicle_LSC_Restriction_Status::Unsupported,
            std::memory_order_release);
        return;
    }

    auto* currentProgram = FindScriptProgram(Joaat("carmod_shop"));
    if (!currentProgram) {
        if (g_lscRuntime.resolved)
            (void)RestorePatchesLocked();
        g_removeLSCRestrictionsStatus.store(
            GTA_Vehicle_LSC_Restriction_Status::WaitingForScript,
            std::memory_order_release);
        return;
    }

    if (g_lscRuntime.resolved &&
        (g_lscRuntime.program != currentProgram || g_lscRuntime.codeBlocks != currentProgram->codeBlocks)) {
        (void)RestorePatchesLocked();
    }

    if (!g_lscRuntime.resolved && !ResolvePatchSitesLocked(currentProgram)) {
        g_removeLSCRestrictionsStatus.store(
            GTA_Vehicle_LSC_Restriction_Status::Unsupported,
            std::memory_order_release);
        return;
    }

    const bool canUsePatched = BytesEqual(
        currentProgram,
        g_lscRuntime.canUseVehicle.pc,
        CanUseVehiclePatch);
    const bool blockMenuPatched = BytesEqual(
        currentProgram,
        g_lscRuntime.blockMenuOption.pc,
        BlockMenuOptionPatch);

    if (!canUsePatched || !blockMenuPatched) {
        const bool originalsPresent =
            BytesEqual(currentProgram, g_lscRuntime.canUseVehicle.pc, g_lscRuntime.canUseVehicle.original) &&
            BytesEqual(currentProgram, g_lscRuntime.blockMenuOption.pc, g_lscRuntime.blockMenuOption.original);
        if (!originalsPresent || !ApplyPatchesLocked()) {
            (void)RestorePatchesLocked();
            g_removeLSCRestrictionsStatus.store(
                GTA_Vehicle_LSC_Restriction_Status::Unsupported,
                std::memory_order_release);
            return;
        }
    } else {
        g_lscRuntime.applied = true;
    }

    g_removeLSCRestrictionsStatus.store(
        GTA_Vehicle_LSC_Restriction_Status::Active,
        std::memory_order_release);
}

[[nodiscard]] int CurrentVehicle(GTA_Native_Manager& natives) noexcept
{
    const auto ped = natives.Invoke<int>(GTA_Native_Id::PlayerPedId);
    if (!ped || *ped == 0)
        return 0;

    const auto seated = natives.Invoke<bool>(GTA_Native_Id::IsPedInAnyVehicle, *ped, false);
    if (!seated || !*seated)
        return 0;

    const auto vehicle = natives.Invoke<int>(GTA_Native_Id::GetVehiclePedIsIn, *ped, false);
    return vehicle && *vehicle != 0 ? *vehicle : 0;
}

[[nodiscard]] bool PrepareCurrentVehicleForLSC(GTA_Native_Manager& natives) noexcept
{
    const int vehicle = CurrentVehicle(natives);
    if (vehicle == 0) {
        g_lscPrepStatus.store(GTA_Vehicle_LSC_Prep_Status::NoVehicle, std::memory_order_release);
        return false;
    }

    const auto model = natives.Invoke<std::uint32_t>(GTA_Native_Id::GetEntityModel, vehicle);
    if (!model || *model == 0) {
        g_lscPrepStatus.store(GTA_Vehicle_LSC_Prep_Status::Failed, std::memory_order_release);
        return false;
    }

    bool success = true;
    success = natives.Invoke<void>(GTA_Native_Id::SetVehicleFixed, vehicle) && success;
    success = natives.Invoke<void>(GTA_Native_Id::SetVehicleDeformationFixed, vehicle) && success;
    success = natives.Invoke<void>(GTA_Native_Id::SetVehicleEngineHealth, vehicle, 1000.0F) && success;
    success = natives.Invoke<void>(GTA_Native_Id::SetVehicleBodyHealth, vehicle, 1000.0F) && success;
    success = natives.Invoke<void>(GTA_Native_Id::SetVehicleDirtLevel, vehicle, 0.0F) && success;
    success = natives.Invoke<void>(GTA_Native_Id::SetVehicleEngineOn, vehicle, true, true, false) && success;

    GTA_Vehicle_State::Instance().RequestForgeSnapshotRefresh();
    g_lscPrepStatus.store(
        success ? GTA_Vehicle_LSC_Prep_Status::Prepared : GTA_Vehicle_LSC_Prep_Status::Failed,
        std::memory_order_release);
    return success;
}
}

void RequestVehicleLSCPrep() noexcept
{
    g_lscPrepStatus.store(GTA_Vehicle_LSC_Prep_Status::Queued, std::memory_order_release);
    g_lscPrepRequested.store(true, std::memory_order_release);
}

GTA_Vehicle_LSC_Prep_Status VehicleLSCPrepStatus() noexcept
{
    return g_lscPrepStatus.load(std::memory_order_acquire);
}

void ConfigureVehicleLSCRestrictions(std::uintptr_t programTableAddress) noexcept
{
    std::lock_guard lock(g_lscRestrictionMutex);
    if (g_lscRuntime.resolved)
        (void)RestorePatchesLocked();
    g_lscRuntime = {};
    g_lscRuntime.programTableAddress = programTableAddress;
    g_removeLSCRestrictionsStatus.store(
        g_removeLSCRestrictions.load(std::memory_order_acquire)
            ? GTA_Vehicle_LSC_Restriction_Status::WaitingForScript
            : GTA_Vehicle_LSC_Restriction_Status::Disabled,
        std::memory_order_release);
}

void ResetVehicleLSCRestrictions() noexcept
{
    std::lock_guard lock(g_lscRestrictionMutex);
    if (g_lscRuntime.resolved)
        (void)RestorePatchesLocked();
    g_lscRuntime = {};
    g_removeLSCRestrictions.store(false, std::memory_order_release);
    g_removeLSCRestrictionsStatus.store(
        GTA_Vehicle_LSC_Restriction_Status::Disabled,
        std::memory_order_release);
}

void SetRemoveLSCRestrictions(bool enabled) noexcept
{
    g_removeLSCRestrictions.store(enabled, std::memory_order_release);
    g_removeLSCRestrictionsStatus.store(
        enabled
            ? GTA_Vehicle_LSC_Restriction_Status::WaitingForScript
            : GTA_Vehicle_LSC_Restriction_Status::Disabled,
        std::memory_order_release);
}

bool RemoveLSCRestrictions() noexcept
{
    return g_removeLSCRestrictions.load(std::memory_order_acquire);
}

GTA_Vehicle_LSC_Restriction_Status RemoveLSCRestrictionsStatus() noexcept
{
    return g_removeLSCRestrictionsStatus.load(std::memory_order_acquire);
}

void TickVehicleGarageSave(GTA_Native_Manager& natives) noexcept
{
    TickLSCRestrictions(natives);

    if (!g_lscPrepRequested.exchange(false, std::memory_order_acq_rel))
        return;

    g_lscPrepStatus.store(GTA_Vehicle_LSC_Prep_Status::Preparing, std::memory_order_release);
    (void)PrepareCurrentVehicleForLSC(natives);
}
}
