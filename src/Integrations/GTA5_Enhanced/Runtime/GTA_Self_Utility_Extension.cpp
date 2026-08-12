#include "GTA_Self_Utility_Extension.hpp"

#include "GTA_Gameplay_State.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Manager.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Registry.hpp"

#include <Windows.h>
#include <Psapi.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

namespace Devilz::Integrations::GTA5_Enhanced
{
namespace
{
constexpr GTA_Native_Hash SetEntityHealthHash = 0xD25E9BDC14A0B649ULL;
constexpr GTA_Native_Hash RestorePlayerStaminaHash = 0x92EBF838856DCF63ULL;
constexpr GTA_Native_Hash GetPedStealthMovementHash = 0xC2BF1F6F84E31EB2ULL;
constexpr GTA_Native_Hash SetPedMoveRateOverrideHash = 0xB27B08E34AC92345ULL;
constexpr GTA_Native_Hash SpecialAbilityUnlockHash = 0xD33BCB9F50C1E588ULL;
constexpr GTA_Native_Hash SpecialAbilityLockHash = 0xE3D5A2DE522F29C1ULL;
constexpr GTA_Native_Hash SetSpecialAbilityMpHash = 0x5F5FDED45A3345C9ULL;

constexpr std::uint32_t TunableBaseAddress = 0x40001U;
constexpr std::size_t TunableScanSlots = 0x20000U;
constexpr ULONGLONG TunableRetryDelayMs = 2000ULL;

constexpr std::array<int, 4> IdleKickDefaults{120000, 300000, 600000, 900000};
constexpr std::array<int, 4> ConstrainedKickDefaults{30000, 60000, 90000, 120000};
constexpr std::array<int, 5> SpecialAbilityCodes{0, 2, 1, 3, 4};
constexpr std::array<const char*, 5> SpecialAbilityLabels{
    "Slipstream",
    "Deadeye",
    "Trevor Rage",
    "Snapshot (Aim at head)",
    "Insult"
};

constexpr std::array<int, 17> ScriptGlobalsPattern{
    0x48, 0x8B, 0x8E, 0xB8, 0x00, 0x00, 0x00, 0x48, 0x8D, 0x15,
    -1, -1, -1, -1, 0x49, 0x89, 0xD8
};

std::atomic_int g_wantedLevel{0};
std::atomic_bool g_wantedRequest{false};
std::atomic_bool g_suicideRequest{false};
std::atomic_bool g_unlimitedStamina{false};
std::atomic_bool g_stealthSpeed{false};
std::atomic<float> g_stealthMultiplier{1.25F};
std::atomic_bool g_specialAbilities{false};
std::atomic_int g_specialAbilitySelection{0};
std::atomic_bool g_noIdleKick{false};
std::atomic_bool g_noIdleKickReady{false};

bool g_specialAbilityApplied = false;
bool g_noIdleKickApplied = false;
std::uintptr_t g_scriptGlobalsAddress = 0;
bool g_scriptGlobalsResolutionAttempted = false;
ULONGLONG g_lastTunableResolveAttempt = 0;
std::array<int*, 8> g_idleKickSlots{};

bool ReadableProtection(DWORD protection) noexcept
{
    protection &= 0xFFU;
    return protection == PAGE_READONLY ||
           protection == PAGE_READWRITE ||
           protection == PAGE_WRITECOPY ||
           protection == PAGE_EXECUTE_READ ||
           protection == PAGE_EXECUTE_READWRITE ||
           protection == PAGE_EXECUTE_WRITECOPY;
}

bool IsReadableAddress(std::uintptr_t address, std::size_t size) noexcept
{
    if (address == 0 || size == 0)
        return false;

    MEMORY_BASIC_INFORMATION memory{};
    if (::VirtualQuery(reinterpret_cast<const void*>(address), &memory, sizeof(memory)) == 0)
        return false;

    if (memory.State != MEM_COMMIT ||
        (memory.Protect & (PAGE_GUARD | PAGE_NOACCESS)) != 0 ||
        !ReadableProtection(memory.Protect)) {
        return false;
    }

    const auto start = reinterpret_cast<std::uintptr_t>(memory.BaseAddress);
    if (start > (std::numeric_limits<std::uintptr_t>::max)() - memory.RegionSize)
        return false;
    const auto end = start + memory.RegionSize;
    return address >= start && address <= end && size <= end - address;
}

bool IsWritableAddress(std::uintptr_t address, std::size_t size) noexcept
{
    if (!IsReadableAddress(address, size))
        return false;

    MEMORY_BASIC_INFORMATION memory{};
    if (::VirtualQuery(reinterpret_cast<const void*>(address), &memory, sizeof(memory)) == 0)
        return false;

    const DWORD protection = memory.Protect & 0xFFU;
    return protection == PAGE_READWRITE ||
           protection == PAGE_WRITECOPY ||
           protection == PAGE_EXECUTE_READWRITE ||
           protection == PAGE_EXECUTE_WRITECOPY;
}

template <std::size_t N>
std::uintptr_t FindPattern(
    std::uintptr_t base,
    std::size_t size,
    const std::array<int, N>& pattern) noexcept
{
    if (base == 0 || size < N || base > (std::numeric_limits<std::uintptr_t>::max)() - size)
        return 0;

    const auto imageEnd = base + size;
    std::uintptr_t cursor = base;
    while (cursor < imageEnd) {
        MEMORY_BASIC_INFORMATION memory{};
        if (::VirtualQuery(reinterpret_cast<const void*>(cursor), &memory, sizeof(memory)) == 0)
            break;

        const auto regionBase = reinterpret_cast<std::uintptr_t>(memory.BaseAddress);
        if (regionBase > (std::numeric_limits<std::uintptr_t>::max)() - memory.RegionSize)
            break;
        const auto regionEnd = (std::min)(imageEnd, regionBase + memory.RegionSize);
        const auto scanBegin = (std::max)(cursor, regionBase);

        const bool readable = memory.State == MEM_COMMIT &&
                              (memory.Protect & (PAGE_GUARD | PAGE_NOACCESS)) == 0 &&
                              ReadableProtection(memory.Protect);
        if (readable && regionEnd > scanBegin && regionEnd - scanBegin >= N) {
            const auto* bytes = reinterpret_cast<const std::uint8_t*>(scanBegin);
            const std::size_t regionSize = regionEnd - scanBegin;
            for (std::size_t offset = 0; offset + N <= regionSize; ++offset) {
                bool match = true;
                for (std::size_t index = 0; index < N; ++index) {
                    if (pattern[index] >= 0 &&
                        bytes[offset + index] != static_cast<std::uint8_t>(pattern[index])) {
                        match = false;
                        break;
                    }
                }
                if (match)
                    return scanBegin + offset;
            }
        }

        if (regionEnd <= cursor)
            break;
        cursor = regionEnd;
    }

    return 0;
}

std::uintptr_t ResolveRipRelative32(std::uintptr_t displacementAddress) noexcept
{
    if (!IsReadableAddress(displacementAddress, sizeof(std::int32_t)))
        return 0;

    std::int32_t displacement = 0;
    std::memcpy(&displacement, reinterpret_cast<const void*>(displacementAddress), sizeof(displacement));
    return static_cast<std::uintptr_t>(
        static_cast<std::intptr_t>(displacementAddress + sizeof(displacement)) + displacement);
}

bool EnsureScriptGlobals() noexcept
{
    if (g_scriptGlobalsAddress != 0)
        return true;
    if (g_scriptGlobalsResolutionAttempted)
        return false;

    g_scriptGlobalsResolutionAttempted = true;
    const HMODULE module = ::GetModuleHandleW(L"GTA5_Enhanced.exe");
    if (!module)
        return false;

    MODULEINFO info{};
    if (::GetModuleInformation(::GetCurrentProcess(), module, &info, sizeof(info)) == FALSE)
        return false;

    const auto base = reinterpret_cast<std::uintptr_t>(info.lpBaseOfDll);
    const auto imageSize = static_cast<std::size_t>(info.SizeOfImage);
    const auto match = FindPattern(base, imageSize, ScriptGlobalsPattern);
    if (match == 0)
        return false;

    g_scriptGlobalsAddress = ResolveRipRelative32(match + 10U);
    return g_scriptGlobalsAddress != 0;
}

bool MatchTunableSequence(
    std::int64_t* start,
    std::size_t offset,
    const std::array<int, 4>& values) noexcept
{
    for (std::size_t index = 0; index < values.size(); ++index) {
        const auto* slot = reinterpret_cast<const int*>(start + offset + index);
        if (*slot != values[index])
            return false;
    }
    return true;
}

bool ResolveNoIdleKickTunables() noexcept
{
    if (g_noIdleKickReady.load(std::memory_order_acquire))
        return true;
    if (!EnsureScriptGlobals() ||
        !IsReadableAddress(g_scriptGlobalsAddress, 64U * sizeof(std::int64_t*))) {
        return false;
    }

    auto** globals = reinterpret_cast<std::int64_t**>(g_scriptGlobalsAddress);
    const std::uint32_t blockIndex = (TunableBaseAddress >> 0x12U) & 0x3FU;
    const std::uint32_t slotIndex = TunableBaseAddress & 0x3FFFFU;

    std::int64_t* block = nullptr;
    std::memcpy(&block, globals + blockIndex, sizeof(block));
    if (!block)
        return false;

    auto* scanStart = block + slotIndex;
    MEMORY_BASIC_INFORMATION memory{};
    if (::VirtualQuery(scanStart, &memory, sizeof(memory)) == 0 ||
        memory.State != MEM_COMMIT ||
        (memory.Protect & (PAGE_GUARD | PAGE_NOACCESS)) != 0 ||
        !ReadableProtection(memory.Protect)) {
        return false;
    }

    const auto regionStart = reinterpret_cast<std::uintptr_t>(scanStart);
    const auto regionEnd = reinterpret_cast<std::uintptr_t>(memory.BaseAddress) + memory.RegionSize;
    if (regionEnd <= regionStart)
        return false;

    const std::size_t availableSlots = (regionEnd - regionStart) / sizeof(std::int64_t);
    const std::size_t scanSlots = (std::min)(TunableScanSlots, availableSlots);
    if (scanSlots < 4)
        return false;

    std::size_t idleOffset = scanSlots;
    std::size_t constrainedOffset = scanSlots;
    for (std::size_t offset = 0; offset + 4 <= scanSlots; ++offset) {
        if (idleOffset == scanSlots && MatchTunableSequence(scanStart, offset, IdleKickDefaults))
            idleOffset = offset;
        if (constrainedOffset == scanSlots && MatchTunableSequence(scanStart, offset, ConstrainedKickDefaults))
            constrainedOffset = offset;
        if (idleOffset != scanSlots && constrainedOffset != scanSlots)
            break;
    }

    if (idleOffset == scanSlots || constrainedOffset == scanSlots)
        return false;

    for (std::size_t index = 0; index < 4; ++index) {
        g_idleKickSlots[index] = reinterpret_cast<int*>(scanStart + idleOffset + index);
        g_idleKickSlots[index + 4] = reinterpret_cast<int*>(scanStart + constrainedOffset + index);
    }

    for (auto* slot : g_idleKickSlots) {
        if (!slot || !IsWritableAddress(reinterpret_cast<std::uintptr_t>(slot), sizeof(int))) {
            g_idleKickSlots.fill(nullptr);
            return false;
        }
    }

    g_noIdleKickReady.store(true, std::memory_order_release);
    return true;
}

void ApplyNoIdleKick(bool enabled) noexcept
{
    if (enabled) {
        if (!ResolveNoIdleKickTunables())
            return;
        for (auto* slot : g_idleKickSlots)
            *slot = (std::numeric_limits<int>::max)();
        g_noIdleKickApplied = true;
        return;
    }

    if (!g_noIdleKickApplied)
        return;

    for (std::size_t index = 0; index < 4; ++index) {
        if (g_idleKickSlots[index] &&
            IsWritableAddress(reinterpret_cast<std::uintptr_t>(g_idleKickSlots[index]), sizeof(int))) {
            *g_idleKickSlots[index] = IdleKickDefaults[index];
        }
        if (g_idleKickSlots[index + 4] &&
            IsWritableAddress(reinterpret_cast<std::uintptr_t>(g_idleKickSlots[index + 4]), sizeof(int))) {
            *g_idleKickSlots[index + 4] = ConstrainedKickDefaults[index];
        }
    }
    g_noIdleKickApplied = false;
}

void TickWantedLevel(GTA_Native_Manager& natives) noexcept
{
    if (!g_wantedRequest.exchange(false, std::memory_order_acq_rel))
        return;

    const auto player = natives.Invoke<int>(GTA_Native_Id::PlayerId);
    if (!player)
        return;

    GTA_Gameplay_State::Instance().SetNeverWanted(false);
    const int level = std::clamp(g_wantedLevel.load(std::memory_order_acquire), 0, 5);
    (void)natives.Invoke<void>(GTA_Native_Id::SetMaxWantedLevel, 6);
    (void)natives.Invoke<void>(GTA_Native_Id::SetPlayerWantedLevel, *player, level, false);
    (void)natives.Invoke<void>(GTA_Native_Id::SetPlayerWantedLevelNow, *player, false);
}

void TickSuicide(GTA_Native_Manager& natives) noexcept
{
    if (!g_suicideRequest.exchange(false, std::memory_order_acq_rel))
        return;

    const auto ped = natives.Invoke<int>(GTA_Native_Id::PlayerPedId);
    if (!ped || *ped == 0)
        return;

    GTA_Gameplay_State::Instance().SetGodMode(false);
    (void)natives.Invoke<void>(GTA_Native_Id::SetEntityInvincible, *ped, false, true);
    (void)natives.InvokeHash<void>(SetEntityHealthHash, *ped, 0, 0);
}

void TickUnlimitedStamina(GTA_Native_Manager& natives) noexcept
{
    if (!g_unlimitedStamina.load(std::memory_order_acquire))
        return;
    const auto player = natives.Invoke<int>(GTA_Native_Id::PlayerId);
    if (player)
        (void)natives.InvokeHash<void>(RestorePlayerStaminaHash, *player, 1.0F);
}

void TickStealthSpeed(GTA_Native_Manager& natives) noexcept
{
    if (!g_stealthSpeed.load(std::memory_order_acquire) || GTA_Gameplay_State::Instance().FastRun())
        return;

    const auto ped = natives.Invoke<int>(GTA_Native_Id::PlayerPedId);
    if (!ped || *ped == 0)
        return;

    const auto stealth = natives.InvokeHash<bool>(GetPedStealthMovementHash, *ped);
    if (!stealth || !*stealth)
        return;

    const float multiplier = std::clamp(g_stealthMultiplier.load(std::memory_order_acquire), 1.0F, 1.49F);
    (void)natives.InvokeHash<void>(SetPedMoveRateOverrideHash, *ped, multiplier);
}

void TickSpecialAbilities(GTA_Native_Manager& natives) noexcept
{
    const bool enabled = g_specialAbilities.load(std::memory_order_acquire);
    if (!enabled) {
        if (g_specialAbilityApplied) {
            (void)natives.InvokeHash<void>(SpecialAbilityLockHash, 0, true);
            g_specialAbilityApplied = false;
        }
        return;
    }

    const auto player = natives.Invoke<int>(GTA_Native_Id::PlayerId);
    if (!player)
        return;

    if (!g_specialAbilityApplied) {
        (void)natives.InvokeHash<void>(SpecialAbilityUnlockHash, 0, true);
        g_specialAbilityApplied = true;
    }

    const int selection = std::clamp(
        g_specialAbilitySelection.load(std::memory_order_acquire),
        0,
        GTA_Self_Special_Ability_Count - 1);
    (void)natives.InvokeHash<void>(SetSpecialAbilityMpHash, *player, SpecialAbilityCodes[selection], 0);
}

void TickNoIdleKick() noexcept
{
    const bool enabled = g_noIdleKick.load(std::memory_order_acquire);
    if (enabled && !g_noIdleKickReady.load(std::memory_order_acquire)) {
        const ULONGLONG now = ::GetTickCount64();
        if (g_lastTunableResolveAttempt == 0 || now - g_lastTunableResolveAttempt >= TunableRetryDelayMs) {
            g_lastTunableResolveAttempt = now;
            (void)ResolveNoIdleKickTunables();
        }
    }
    ApplyNoIdleKick(enabled);
}
}

int SelfWantedLevelSelection() noexcept
{
    return g_wantedLevel.load(std::memory_order_acquire);
}

void SetSelfWantedLevelSelection(int level) noexcept
{
    g_wantedLevel.store(std::clamp(level, 0, 5), std::memory_order_release);
}

void RequestSelfWantedLevel() noexcept
{
    g_wantedRequest.store(true, std::memory_order_release);
}

void RequestSelfSuicide() noexcept
{
    g_suicideRequest.store(true, std::memory_order_release);
}

bool SelfUnlimitedStamina() noexcept
{
    return g_unlimitedStamina.load(std::memory_order_acquire);
}

void SetSelfUnlimitedStamina(bool enabled) noexcept
{
    g_unlimitedStamina.store(enabled, std::memory_order_release);
}

bool SelfStealthSpeed() noexcept
{
    return g_stealthSpeed.load(std::memory_order_acquire);
}

void SetSelfStealthSpeed(bool enabled) noexcept
{
    g_stealthSpeed.store(enabled, std::memory_order_release);
}

float SelfStealthSpeedMultiplier() noexcept
{
    return g_stealthMultiplier.load(std::memory_order_acquire);
}

void SetSelfStealthSpeedMultiplier(float multiplier) noexcept
{
    g_stealthMultiplier.store(std::clamp(multiplier, 1.0F, 1.49F), std::memory_order_release);
}

bool SelfSpecialAbilities() noexcept
{
    return g_specialAbilities.load(std::memory_order_acquire);
}

void SetSelfSpecialAbilities(bool enabled) noexcept
{
    g_specialAbilities.store(enabled, std::memory_order_release);
}

int SelfSpecialAbilitySelection() noexcept
{
    return g_specialAbilitySelection.load(std::memory_order_acquire);
}

void SetSelfSpecialAbilitySelection(int index) noexcept
{
    g_specialAbilitySelection.store(
        std::clamp(index, 0, GTA_Self_Special_Ability_Count - 1),
        std::memory_order_release);
}

const char* SelfSpecialAbilityLabel(int index) noexcept
{
    if (index < 0 || index >= GTA_Self_Special_Ability_Count)
        return "Unknown";
    return SpecialAbilityLabels[static_cast<std::size_t>(index)];
}

bool SelfNoIdleKick() noexcept
{
    return g_noIdleKick.load(std::memory_order_acquire);
}

void SetSelfNoIdleKick(bool enabled) noexcept
{
    g_noIdleKick.store(enabled, std::memory_order_release);
}

bool SelfNoIdleKickReady() noexcept
{
    return g_noIdleKickReady.load(std::memory_order_acquire);
}

void ResetSelfUtilityExtension() noexcept
{
    ApplyNoIdleKick(false);
    g_wantedLevel.store(0, std::memory_order_release);
    g_wantedRequest.store(false, std::memory_order_release);
    g_suicideRequest.store(false, std::memory_order_release);
    g_unlimitedStamina.store(false, std::memory_order_release);
    g_stealthSpeed.store(false, std::memory_order_release);
    g_stealthMultiplier.store(1.25F, std::memory_order_release);
    g_specialAbilities.store(false, std::memory_order_release);
    g_specialAbilitySelection.store(0, std::memory_order_release);
    g_noIdleKick.store(false, std::memory_order_release);
    g_noIdleKickReady.store(false, std::memory_order_release);
    g_specialAbilityApplied = false;
    g_noIdleKickApplied = false;
    g_scriptGlobalsAddress = 0;
    g_scriptGlobalsResolutionAttempted = false;
    g_lastTunableResolveAttempt = 0;
    g_idleKickSlots.fill(nullptr);
}

void TickSelfUtilityExtension(GTA_Native_Manager& natives) noexcept
{
    TickWantedLevel(natives);
    TickSuicide(natives);
    TickUnlimitedStamina(natives);
    TickStealthSpeed(natives);
    TickSpecialAbilities(natives);
    TickNoIdleKick();
}
}
