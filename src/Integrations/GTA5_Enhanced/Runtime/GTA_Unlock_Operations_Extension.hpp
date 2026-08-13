#pragma once

#include "GTA_Unlock_Operations_State.hpp"

#include <Windows.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <string_view>

namespace Devilz::Integrations::GTA5_Enhanced
{
namespace UnlockOperationsDetail
{
inline constexpr std::uint64_t StatGetInt = 0x767FBC2AC802EF3DULL;
inline constexpr std::uint64_t StatGetBool = 0x11B5E6D2AE73F48EULL;
inline constexpr std::uint64_t StatSetInt = 0x1164A75E490C27B6ULL;
inline constexpr std::uint64_t StatSetBool = 0xF1D0B0CE940F620DULL;
inline constexpr std::uint64_t GetPackedStatBoolCode = 0xA6D3C21763E25496ULL;
inline constexpr std::uint64_t SetPackedStatBoolCode = 0xA595AA1819B05EA0ULL;
inline constexpr std::array<int, 17> ScriptGlobalsPattern{
    0x48, 0x8B, 0x8E, 0xB8, 0x00, 0x00, 0x00, 0x48, 0x8D, 0x15,
    -1, -1, -1, -1, 0x49, 0x89, 0xD8
};
inline constexpr std::size_t CommandsPerTick = 8;

inline std::uintptr_t g_scriptGlobals = 0;
inline bool g_scriptGlobalsScanned = false;

[[nodiscard]] inline bool IsReadable(std::uintptr_t address, std::size_t size) noexcept
{
    if (!address || !size || address > (std::numeric_limits<std::uintptr_t>::max)() - size)
        return false;

    MEMORY_BASIC_INFORMATION memory{};
    if (!::VirtualQuery(reinterpret_cast<const void*>(address), &memory, sizeof(memory)))
        return false;
    if (memory.State != MEM_COMMIT || (memory.Protect & (PAGE_GUARD | PAGE_NOACCESS)) != 0)
        return false;

    const auto protection = memory.Protect & 0xFFU;
    const bool readable = protection == PAGE_READONLY || protection == PAGE_READWRITE ||
        protection == PAGE_WRITECOPY || protection == PAGE_EXECUTE_READ ||
        protection == PAGE_EXECUTE_READWRITE || protection == PAGE_EXECUTE_WRITECOPY;
    if (!readable)
        return false;

    const auto begin = reinterpret_cast<std::uintptr_t>(memory.BaseAddress);
    if (begin > (std::numeric_limits<std::uintptr_t>::max)() - memory.RegionSize)
        return false;
    const auto end = begin + memory.RegionSize;
    return address >= begin && address <= end && size <= end - address;
}

[[nodiscard]] inline bool IsWritable(std::uintptr_t address, std::size_t size) noexcept
{
    if (!IsReadable(address, size))
        return false;
    MEMORY_BASIC_INFORMATION memory{};
    if (!::VirtualQuery(reinterpret_cast<const void*>(address), &memory, sizeof(memory)))
        return false;
    const auto protection = memory.Protect & 0xFFU;
    return protection == PAGE_READWRITE || protection == PAGE_WRITECOPY ||
        protection == PAGE_EXECUTE_READWRITE || protection == PAGE_EXECUTE_WRITECOPY;
}

[[nodiscard]] inline bool PatternAt(const std::uint8_t* bytes) noexcept
{
    for (std::size_t index = 0; index < ScriptGlobalsPattern.size(); ++index) {
        if (ScriptGlobalsPattern[index] >= 0 &&
            bytes[index] != static_cast<std::uint8_t>(ScriptGlobalsPattern[index])) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] inline std::uintptr_t ResolveScriptGlobals() noexcept
{
    if (g_scriptGlobals || g_scriptGlobalsScanned)
        return g_scriptGlobals;
    g_scriptGlobalsScanned = true;

    const auto module = ::GetModuleHandleW(L"GTA5_Enhanced.exe");
    if (!module)
        return 0;
    const auto base = reinterpret_cast<std::uintptr_t>(module);
    if (!IsReadable(base, sizeof(IMAGE_DOS_HEADER)))
        return 0;
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0)
        return 0;

    const auto ntAddress = base + static_cast<std::uintptr_t>(dos->e_lfanew);
    if (!IsReadable(ntAddress, sizeof(IMAGE_NT_HEADERS64)))
        return 0;
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(ntAddress);
    if (nt->Signature != IMAGE_NT_SIGNATURE)
        return 0;

    std::uintptr_t match = 0;
    std::size_t matchCount = 0;
    const auto* sections = IMAGE_FIRST_SECTION(nt);
    for (std::uint16_t sectionIndex = 0; sectionIndex < nt->FileHeader.NumberOfSections; ++sectionIndex) {
        const auto& section = sections[sectionIndex];
        if ((section.Characteristics & IMAGE_SCN_MEM_EXECUTE) == 0)
            continue;
        const auto start = base + section.VirtualAddress;
        const auto size = static_cast<std::size_t>(section.Misc.VirtualSize);
        if (size < ScriptGlobalsPattern.size() || !IsReadable(start, size))
            continue;

        const auto* bytes = reinterpret_cast<const std::uint8_t*>(start);
        for (std::size_t offset = 0; offset + ScriptGlobalsPattern.size() <= size; ++offset) {
            if (!PatternAt(bytes + offset))
                continue;
            match = start + offset;
            if (++matchCount > 1)
                return 0;
        }
    }

    if (matchCount != 1 || !IsReadable(match + 10, sizeof(std::int32_t)))
        return 0;

    std::int32_t displacement = 0;
    std::memcpy(&displacement, reinterpret_cast<const void*>(match + 10), sizeof(displacement));
    const auto resolvedSigned = static_cast<std::intptr_t>(match + 14) + displacement;
    if (resolvedSigned <= 0)
        return 0;

    const auto resolved = static_cast<std::uintptr_t>(resolvedSigned);
    if (!IsReadable(resolved, 64 * sizeof(std::int64_t*)))
        return 0;
    g_scriptGlobals = resolved;
    return g_scriptGlobals;
}

[[nodiscard]] inline std::int32_t* ResolveGlobalInt(std::int32_t globalIndex) noexcept
{
    if (globalIndex < 0)
        return nullptr;
    const auto globalsAddress = ResolveScriptGlobals();
    if (!globalsAddress)
        return nullptr;

    const auto index = static_cast<std::uint32_t>(globalIndex);
    const auto blockIndex = (index >> 0x12U) & 0x3FU;
    const auto slotIndex = index & 0x3FFFFU;
    auto** globals = reinterpret_cast<std::int64_t**>(globalsAddress);

    std::int64_t* block = nullptr;
    std::memcpy(&block, globals + blockIndex, sizeof(block));
    if (!block)
        return nullptr;

    auto* destination = reinterpret_cast<std::int32_t*>(block + slotIndex);
    if (!IsReadable(reinterpret_cast<std::uintptr_t>(destination), sizeof(*destination)))
        return nullptr;
    return destination;
}

[[nodiscard]] inline char Lower(char value) noexcept
{
    return value >= 'A' && value <= 'Z' ? static_cast<char>(value + ('a' - 'A')) : value;
}

[[nodiscard]] inline std::uint32_t Joaat(std::string_view text) noexcept
{
    std::uint32_t hash = 0;
    for (char character : text) {
        hash += static_cast<std::uint8_t>(Lower(character));
        hash += hash << 10U;
        hash ^= hash >> 6U;
    }
    hash += hash << 3U;
    hash ^= hash >> 11U;
    return hash + (hash << 15U);
}

template <typename NativeManager>
[[nodiscard]] bool ResolveStatHash(NativeManager& natives, std::string name, std::uint32_t& hash) noexcept
{
    if (name.empty() || name.size() > 127)
        return false;

    if (name.size() > 3 && Lower(name[0]) == 'm' && Lower(name[1]) == 'p' &&
        Lower(name[2]) == 'x' && name[3] == '_') {
        std::int32_t character = 0;
        const auto result = natives.template InvokeHash<bool>(
            StatGetInt,
            Joaat("MPPLY_LAST_MP_CHAR"),
            &character,
            -1);
        if (!result.has_value() || !result.value())
            return false;
        name[2] = character == 1 ? '1' : '0';
    }

    hash = Joaat(name);
    return hash != 0;
}

inline void CompleteFailure(
    GTA_Unlock_Operations_State& state,
    const GTA_Unlock_Operations_State::Command& command,
    const char* detail) noexcept
{
    state.Complete(command, false, false, false, detail ? detail : "Unlock operation failed");
}

template <typename NativeManager>
void ExecuteCommand(
    NativeManager& natives,
    GTA_Unlock_Operations_State& state,
    const GTA_Unlock_Operations_State::Command& command) noexcept
{
    const auto& operation = command.operation;
    const bool write = command.kind == GTA_Unlock_Operations_State::Command_Kind::Write;

    if (operation.type == GTA_Unlock_Operation_Type::PackedBool) {
        if (!write) {
            const auto current = natives.template InvokeHash<bool>(GetPackedStatBoolCode, operation.index, -1);
            if (!current.has_value()) {
                CompleteFailure(state, command, "Packed-stat read handler unavailable");
                return;
            }
            state.Complete(command, true, true, current.value() == operation.boolValue, "Packed stat read");
            return;
        }

        const bool success = natives.template InvokeHash<void>(
            SetPackedStatBoolCode,
            operation.index,
            operation.boolValue,
            -1);
        state.Complete(command, success, success, success,
            success ? "Packed stat updated" : "Packed-stat write handler unavailable");
        return;
    }

    if (operation.type == GTA_Unlock_Operation_Type::TunableInt) {
        auto* value = ResolveGlobalInt(operation.index);
        if (!value) {
            CompleteFailure(state, command, "Script global/tunable is unavailable on this Enhanced build");
            return;
        }
        if (!write) {
            state.Complete(command, true, true, *value == operation.intValue, "Tunable read");
            return;
        }
        if (!IsWritable(reinterpret_cast<std::uintptr_t>(value), sizeof(*value))) {
            CompleteFailure(state, command, "Script global/tunable is not writable");
            return;
        }
        *value = operation.intValue;
        state.Complete(command, true, true, true, "Tunable updated");
        return;
    }

    std::uint32_t statHash = 0;
    if (!ResolveStatHash(natives, operation.statName, statHash)) {
        CompleteFailure(state, command, "Stat name could not be resolved");
        return;
    }

    if (operation.type == GTA_Unlock_Operation_Type::StatBool) {
        std::int32_t current = 0;
        const auto read = natives.template InvokeHash<bool>(StatGetBool, statHash, &current, -1);
        if (!read.has_value() || !read.value()) {
            CompleteFailure(state, command, "STAT_GET_BOOL failed for this stat");
            return;
        }
        if (!write) {
            state.Complete(command, true, true, (current != 0) == operation.boolValue, "Stat bool read");
            return;
        }
        const bool success = natives.template InvokeHash<void>(StatSetBool, statHash, operation.boolValue, true);
        state.Complete(command, success, success, success,
            success ? "Stat bool updated" : "STAT_SET_BOOL handler unavailable");
        return;
    }

    if (operation.type == GTA_Unlock_Operation_Type::StatInt) {
        std::int32_t current = 0;
        const auto read = natives.template InvokeHash<bool>(StatGetInt, statHash, &current, -1);
        if (!read.has_value() || !read.value()) {
            CompleteFailure(state, command, "STAT_GET_INT failed for this stat");
            return;
        }
        if (!write) {
            state.Complete(command, true, true, current == operation.intValue, "Stat int read");
            return;
        }
        const bool success = natives.template InvokeHash<void>(StatSetInt, statHash, operation.intValue, true);
        state.Complete(command, success, success, success,
            success ? "Stat int updated" : "STAT_SET_INT handler unavailable");
        return;
    }

    CompleteFailure(state, command, "Unsupported unlock operation type");
}
}

template <typename NativeManager>
void TickUnlockOperationsExtension(NativeManager& natives) noexcept
{
    auto& state = GTA_Unlock_Operations_State::Instance();
    for (std::size_t processed = 0; processed < UnlockOperationsDetail::CommandsPerTick; ++processed) {
        GTA_Unlock_Operations_State::Command command{};
        if (!state.Consume(command))
            break;
        try {
            UnlockOperationsDetail::ExecuteCommand(natives, state, command);
        } catch (...) {
            UnlockOperationsDetail::CompleteFailure(
                state,
                command,
                "Unlock operation threw on the GTA game thread");
        }
    }
}
}
