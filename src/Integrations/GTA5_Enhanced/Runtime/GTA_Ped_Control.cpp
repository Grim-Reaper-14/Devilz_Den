#include "GTA_Ped_Control.hpp"

#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Manager.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Registry.hpp"

#include <Windows.h>
#include <intrin.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

namespace Devilz::Integrations::GTA5_Enhanced
{
namespace
{
constexpr std::array<std::uint8_t, 13> PedPoolPattern{
    0x80, 0x79, 0x4B, 0x00, 0x0F, 0x84, 0xF5, 0x00, 0x00, 0x00, 0x48, 0x89, 0xF1
};
constexpr std::array<std::uint8_t, 14> HandlesAndPtrsPattern{
    0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00, 0x89, 0xF8, 0x0F, 0x28, 0xFE, 0x41
};
constexpr std::size_t PedPoolRipInstructionOffset = 0x18;
constexpr std::size_t PtrToHandleCallBackOffset = 0x0B;
constexpr GTA_Native_Hash IsPedAPlayer = 0x501EBB0523078750ULL;
constexpr GTA_Native_Hash IsPedInCombat = 0x1B32E388988DD296ULL;
constexpr GTA_Native_Hash GetRelationshipBetweenPeds = 0x1E37AEC038A241A3ULL;
constexpr GTA_Native_Hash SetEntityHealth = 0xD25E9BDC14A0B649ULL;
std::atomic_bool g_pedControlBusy{false};

struct PoolEncryption
{
    bool isSet = false;
    std::byte pad[7]{};
    std::uint64_t first = 0;
    std::uint64_t second = 0;
};
static_assert(sizeof(PoolEncryption) == 0x18);

using PtrToHandle = int (*)(void*);

[[nodiscard]] bool Readable(const void* address, std::size_t bytes) noexcept;

struct BasePool
{
    void* vtable = nullptr;
    std::uintptr_t entries = 0;
    std::uint8_t* flags = nullptr;
    std::uint32_t size = 0;
    std::uint32_t itemSize = 0;
    std::uint32_t nextSlotIndex = 0;
    std::uint32_t unknown24 = 0;
    std::uint32_t freeSlotIndex = 0;

    [[nodiscard]] bool IsValid(std::uint32_t index) const noexcept
    {
        return flags && index < size && (flags[index] & 0x80U) == 0;
    }

    [[nodiscard]] void* GetAt(std::uint32_t index) const noexcept
    {
        if (!IsValid(index) || flags[index] == 0 || entries == 0 || itemSize == 0)
            return nullptr;
        if (index > ((std::numeric_limits<std::uintptr_t>::max)() - entries) / itemSize)
            return nullptr;
        const auto address = entries + static_cast<std::uintptr_t>(index) * itemSize;
        if (!Readable(reinterpret_cast<const void*>(address), 0x18))
            return nullptr;
        const auto objectMarker = *reinterpret_cast<void* const*>(address + 0x10);
        return objectMarker ? reinterpret_cast<void*>(address) : nullptr;
    }
};
static_assert(sizeof(BasePool) == 0x30);

[[nodiscard]] bool Readable(const void* address, std::size_t bytes) noexcept
{
    if (!address || bytes == 0)
        return false;
    MEMORY_BASIC_INFORMATION memory{};
    if (::VirtualQuery(address, &memory, sizeof(memory)) == 0)
        return false;
    if (memory.State != MEM_COMMIT || (memory.Protect & (PAGE_GUARD | PAGE_NOACCESS)) != 0)
        return false;
    const auto begin = reinterpret_cast<std::uintptr_t>(memory.BaseAddress);
    const auto current = reinterpret_cast<std::uintptr_t>(address);
    if (begin > (std::numeric_limits<std::uintptr_t>::max)() - memory.RegionSize)
        return false;
    const auto end = begin + memory.RegionSize;
    return current >= begin && current <= end && bytes <= end - current;
}

[[nodiscard]] bool Executable(const void* address) noexcept
{
    if (!address)
        return false;
    MEMORY_BASIC_INFORMATION memory{};
    if (::VirtualQuery(address, &memory, sizeof(memory)) == 0)
        return false;
    if (memory.State != MEM_COMMIT || (memory.Protect & (PAGE_GUARD | PAGE_NOACCESS)) != 0)
        return false;
    const DWORD protection = memory.Protect & 0xFFU;
    return protection == PAGE_EXECUTE || protection == PAGE_EXECUTE_READ ||
           protection == PAGE_EXECUTE_READWRITE || protection == PAGE_EXECUTE_WRITECOPY;
}

[[nodiscard]] std::size_t ImageSize(std::uintptr_t moduleBase) noexcept
{
    if (!Readable(reinterpret_cast<const void*>(moduleBase), sizeof(IMAGE_DOS_HEADER)))
        return 0;
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(moduleBase);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0)
        return 0;
    const auto ntAddress = moduleBase + static_cast<std::uintptr_t>(dos->e_lfanew);
    if (!Readable(reinterpret_cast<const void*>(ntAddress), sizeof(IMAGE_NT_HEADERS64)))
        return 0;
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(ntAddress);
    return nt->Signature == IMAGE_NT_SIGNATURE ? nt->OptionalHeader.SizeOfImage : 0;
}

struct PedRuntimePointers
{
    PoolEncryption* encryptedPool = nullptr;
    PtrToHandle ptrToHandle = nullptr;
};

[[nodiscard]] const PedRuntimePointers& ResolvePedRuntimePointers() noexcept
{
    static PedRuntimePointers cached{};
    static bool attempted = false;
    if (attempted)
        return cached;
    attempted = true;

    const auto module = ::GetModuleHandleW(L"GTA5_Enhanced.exe");
    const auto base = reinterpret_cast<std::uintptr_t>(module);
    const auto size = ImageSize(base);
    const std::size_t minimumPatternSize = (std::min)(PedPoolPattern.size(), HandlesAndPtrsPattern.size());
    if (!base || size < minimumPatternSize || base > (std::numeric_limits<std::uintptr_t>::max)() - size)
        return cached;

    const auto moduleEnd = base + size;
    std::uintptr_t cursor = base;
    while (cursor < moduleEnd) {
        MEMORY_BASIC_INFORMATION memory{};
        if (::VirtualQuery(reinterpret_cast<const void*>(cursor), &memory, sizeof(memory)) == 0)
            return cached;
        const auto regionBase = reinterpret_cast<std::uintptr_t>(memory.BaseAddress);
        if (regionBase > (std::numeric_limits<std::uintptr_t>::max)() - memory.RegionSize)
            return cached;
        const auto regionEndRaw = regionBase + memory.RegionSize;
        const auto regionStart = (std::max)(cursor, regionBase);
        const auto regionEnd = (std::min)(moduleEnd, regionEndRaw);

        if (memory.State == MEM_COMMIT && (memory.Protect & (PAGE_GUARD | PAGE_NOACCESS)) == 0 && regionEnd > regionStart) {
            const auto* bytes = reinterpret_cast<const std::uint8_t*>(regionStart);
            const std::size_t regionSize = regionEnd - regionStart;
            for (std::size_t i = 0; i < regionSize; ++i) {
                if (!cached.encryptedPool && i + PedPoolPattern.size() <= regionSize &&
                    std::memcmp(bytes + i, PedPoolPattern.data(), PedPoolPattern.size()) == 0) {
                    const auto match = regionStart + i;
                    if (match <= (std::numeric_limits<std::uintptr_t>::max)() - PedPoolRipInstructionOffset) {
                        const auto instruction = match + PedPoolRipInstructionOffset;
                        if (instruction <= (std::numeric_limits<std::uintptr_t>::max)() - 7 &&
                            Readable(reinterpret_cast<const void*>(instruction + 3), sizeof(std::int32_t))) {
                            std::int32_t displacement = 0;
                            std::memcpy(&displacement, reinterpret_cast<const void*>(instruction + 3), sizeof(displacement));
                            const auto targetSigned = static_cast<std::intptr_t>(instruction + 7) + static_cast<std::intptr_t>(displacement);
                            if (targetSigned > 0) {
                                auto* encrypted = reinterpret_cast<PoolEncryption*>(static_cast<std::uintptr_t>(targetSigned));
                                if (Readable(encrypted, sizeof(PoolEncryption)))
                                    cached.encryptedPool = encrypted;
                            }
                        }
                    }
                }

                if (!cached.ptrToHandle && i + HandlesAndPtrsPattern.size() <= regionSize &&
                    std::memcmp(bytes + i, HandlesAndPtrsPattern.data(), HandlesAndPtrsPattern.size()) == 0) {
                    const auto match = regionStart + i;
                    if (match >= base + PtrToHandleCallBackOffset) {
                        const auto callInstruction = match - PtrToHandleCallBackOffset;
                        if (Readable(reinterpret_cast<const void*>(callInstruction), 5) &&
                            *reinterpret_cast<const std::uint8_t*>(callInstruction) == 0xE8U) {
                            std::int32_t displacement = 0;
                            std::memcpy(&displacement, reinterpret_cast<const void*>(callInstruction + 1), sizeof(displacement));
                            const auto targetSigned = static_cast<std::intptr_t>(callInstruction + 5) + static_cast<std::intptr_t>(displacement);
                            if (targetSigned > 0) {
                                const auto target = static_cast<std::uintptr_t>(targetSigned);
                                if (target >= base && target < moduleEnd && Executable(reinterpret_cast<const void*>(target)))
                                    cached.ptrToHandle = reinterpret_cast<PtrToHandle>(target);
                            }
                        }
                    }
                }

                if (cached.encryptedPool && cached.ptrToHandle)
                    return cached;
            }
        }
        if (regionEnd <= cursor)
            return cached;
        cursor = regionEnd;
    }
    return cached;
}

[[nodiscard]] BasePool* PedPool() noexcept
{
    auto* encrypted = ResolvePedRuntimePointers().encryptedPool;
    if (!encrypted || !encrypted->isSet)
        return nullptr;
    const std::uint64_t x = _rotl64(encrypted->second, 30);
    const std::uint64_t decoded = ~_rotl64(_rotl64(x ^ encrypted->first, 32), (static_cast<std::uint8_t>(x) & 0x1FU) + 2U);
    auto* pool = reinterpret_cast<BasePool*>(decoded);
    if (!Readable(pool, sizeof(BasePool)) || pool->size == 0 || pool->size > 8192 || pool->entries == 0 ||
        pool->itemSize < 0x18 || pool->itemSize > 0x1000 || !pool->flags || !Readable(pool->flags, pool->size))
        return nullptr;
    return pool;
}

[[nodiscard]] PtrToHandle PedPtrToHandle() noexcept
{
    return ResolvePedRuntimePointers().ptrToHandle;
}

[[nodiscard]] float DistanceSquared(const GTA_Native_Script_Vector& a, const GTA_Native_Script_Vector& b) noexcept
{
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    const float dz = a.z - b.z;
    return dx * dx + dy * dy + dz * dz;
}
}

GTA_Ped_Control_State& GTA_Ped_Control_State::Instance() noexcept
{
    static GTA_Ped_Control_State state;
    return state;
}

void GTA_Ped_Control_State::SetRadius(float radius) noexcept { m_radius.store(std::clamp(radius, 10.0F, 1000.0F), std::memory_order_release); }
float GTA_Ped_Control_State::Radius() const noexcept { return m_radius.load(std::memory_order_acquire); }
void GTA_Ped_Control_State::RequestKillEnemies() noexcept { g_pedControlBusy.store(true, std::memory_order_release); m_action.store(GTA_Ped_Action::KillEnemies, std::memory_order_release); }
void GTA_Ped_Control_State::RequestKillPeds() noexcept { g_pedControlBusy.store(true, std::memory_order_release); m_action.store(GTA_Ped_Action::KillPeds, std::memory_order_release); }
GTA_Ped_Action GTA_Ped_Control_State::ConsumeAction() noexcept { return m_action.exchange(GTA_Ped_Action::None, std::memory_order_acq_rel); }

void GTA_Ped_Control_State::Publish(GTA_Ped_Action_Snapshot snapshot) noexcept
{
    m_lastAction.store(snapshot.action, std::memory_order_release);
    m_affected.store(snapshot.affected, std::memory_order_release);
    m_skippedPlayers.store(snapshot.skippedPlayers, std::memory_order_release);
    m_poolReady.store(snapshot.poolReady, std::memory_order_release);
}

GTA_Ped_Action_Snapshot GTA_Ped_Control_State::Snapshot() const noexcept
{
    GTA_Ped_Action_Snapshot snapshot{};
    snapshot.action = m_lastAction.load(std::memory_order_acquire);
    snapshot.affected = m_affected.load(std::memory_order_acquire);
    snapshot.skippedPlayers = m_skippedPlayers.load(std::memory_order_acquire);
    snapshot.poolReady = m_poolReady.load(std::memory_order_acquire);
    return snapshot;
}

void GTA_Ped_Control_State::Reset() noexcept
{
    g_pedControlBusy.store(false, std::memory_order_release);
    m_radius.store(150.0F, std::memory_order_release);
    m_action.store(GTA_Ped_Action::None, std::memory_order_release);
    Publish({});
}

void TickPedControl(GTA_Native_Manager& natives) noexcept
{
    struct PendingScan
    {
        GTA_Ped_Action action = GTA_Ped_Action::None;
        BasePool* pool = nullptr;
        PtrToHandle ptrToHandle = nullptr;
        std::uint32_t nextIndex = 0;
        int playerPed = 0;
        GTA_Native_Script_Vector playerCoords{};
        float radiusSquared = 0.0F;
        GTA_Ped_Action_Snapshot result{};
    };

    constexpr std::uint32_t PoolSlotsPerTick = 64;
    static PendingScan scan{};
    auto& state = GTA_Ped_Control_State::Instance();
    const auto requested = state.ConsumeAction();
    if (requested != GTA_Ped_Action::None) {
        scan = {};
        scan.action = requested;
        scan.result.action = requested;
        scan.pool = PedPool();
        scan.ptrToHandle = PedPtrToHandle();
        scan.result.poolReady = scan.pool != nullptr && scan.ptrToHandle != nullptr;
        if (!scan.pool || !scan.ptrToHandle) {
            state.Publish(scan.result);
            scan = {};
            g_pedControlBusy.store(false, std::memory_order_release);
            return;
        }
        const auto playerPed = natives.Invoke<int>(GTA_Native_Id::PlayerPedId);
        if (!playerPed || *playerPed == 0) {
            state.Publish(scan.result);
            scan = {};
            g_pedControlBusy.store(false, std::memory_order_release);
            return;
        }
        scan.playerPed = *playerPed;
        const auto playerCoords = natives.Invoke<GTA_Native_Script_Vector>(GTA_Native_Id::GetEntityCoords, scan.playerPed, true);
        if (!playerCoords) {
            state.Publish(scan.result);
            scan = {};
            g_pedControlBusy.store(false, std::memory_order_release);
            return;
        }
        scan.playerCoords = *playerCoords;
        const float radius = state.Radius();
        scan.radiusSquared = radius * radius;
    }

    if (scan.action == GTA_Ped_Action::None || !scan.pool)
        return;

    const std::uint32_t endIndex = (std::min)(scan.pool->size, scan.nextIndex + PoolSlotsPerTick);
    for (; scan.nextIndex < endIndex; ++scan.nextIndex) {
        const std::uint32_t index = scan.nextIndex;
        if (!scan.pool->IsValid(index))
            continue;
        void* pedPointer = scan.pool->GetAt(index);
        if (!pedPointer)
            continue;
        const int ped = scan.ptrToHandle(pedPointer);
        if (ped == 0 || ped == scan.playerPed)
            continue;
        const auto isPlayer = natives.InvokeHash<bool>(IsPedAPlayer, ped);
        if (!isPlayer)
            continue;
        if (*isPlayer) {
            ++scan.result.skippedPlayers;
            continue;
        }
        const auto coords = natives.Invoke<GTA_Native_Script_Vector>(GTA_Native_Id::GetEntityCoords, ped, true);
        if (!coords || DistanceSquared(*coords, scan.playerCoords) > scan.radiusSquared)
            continue;
        if (scan.action == GTA_Ped_Action::KillEnemies) {
            const auto relationshipToPlayer = natives.InvokeHash<int>(GetRelationshipBetweenPeds, ped, scan.playerPed);
            const auto relationshipFromPlayer = natives.InvokeHash<int>(GetRelationshipBetweenPeds, scan.playerPed, ped);
            const auto inCombat = natives.InvokeHash<bool>(IsPedInCombat, ped, scan.playerPed);
            const bool hostileRelationship =
                (relationshipToPlayer && *relationshipToPlayer >= 3 && *relationshipToPlayer <= 5) ||
                (relationshipFromPlayer && *relationshipFromPlayer >= 3 && *relationshipFromPlayer <= 5);
            if (!hostileRelationship && (!inCombat || !*inCombat))
                continue;
        }
        if (natives.InvokeHash<void>(SetEntityHealth, ped, 0, scan.playerPed, 0))
            ++scan.result.affected;
    }

    if (scan.nextIndex >= scan.pool->size) {
        state.Publish(scan.result);
        scan = {};
        g_pedControlBusy.store(false, std::memory_order_release);
    }
}

bool PedControlHasWork() noexcept { return g_pedControlBusy.load(std::memory_order_acquire); }
void ResetPedControl() noexcept { GTA_Ped_Control_State::Instance().Reset(); }
}
