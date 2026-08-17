#include "GTA_Run_Script_Threads_Bridge.hpp"

#include "GTA_Bunker_Extension.hpp"
#include "GTA_Business_Extension.hpp"
#include "GTA_Casino_Extension.hpp"
#include "GTA_Explosive_Ammo_Extension.hpp"
#include "GTA_Gameplay_State.hpp"
#include "GTA_Network_Session_Extension.hpp"
#include "GTA_Ped_Control.hpp"
#include "GTA_Random_Events_Extension.hpp"
#include "GTA_Script_Function_Invoker.hpp"
#include "GTA_Self_Online_Extension.hpp"
#include "GTA_Self_Utility_Extension.hpp"
#include "GTA_Stats_Extension.hpp"
#include "GTA_Vehicle_Editor_Extensions.hpp"
#include "GTA_Vehicle_Personal_Save.hpp"
#include "GTA_Vehicle_State.hpp"
#include "State/GTA_Self_Cache.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Registry.hpp"

#include <intrin.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <string>

namespace Devilz::Integrations::GTA5_Enhanced
{
namespace
{
struct GTA_Tls_Context_View
{
    std::byte pad00[0x7A0]{};
    void* currentScriptThread = nullptr;
    bool scriptThreadActive = false;
};

static_assert(offsetof(GTA_Tls_Context_View, currentScriptThread) == 0x7A0);
static_assert(offsetof(GTA_Tls_Context_View, scriptThreadActive) == 0x7A8);

constexpr std::array<std::byte, GTA_Run_Script_Threads_Bridge::PatchSize> VerifiedPrologue{
    std::byte{0x56}, std::byte{0x57}, std::byte{0x55}, std::byte{0x53},
    std::byte{0x48}, std::byte{0x83}, std::byte{0xEC}, std::byte{0x28},
    std::byte{0x85}, std::byte{0xC9}, std::byte{0xBE}, std::byte{0x40},
    std::byte{0x5D}, std::byte{0xC6}, std::byte{0x00}
};
constexpr ULONGLONG FeatureTickIntervalMs = 16;
constexpr ULONGLONG ScheduledDispatchGapMs = 8;
constexpr ULONGLONG GameplayTickIntervalMs = 100;
constexpr ULONGLONG IdleGameplayTickIntervalMs = 500;
constexpr ULONGLONG SelfCacheTickIntervalMs = 250;
constexpr ULONGLONG SelfUtilityTickIntervalMs = 100;
constexpr ULONGLONG OnlineExtensionTickIntervalMs = 250;
constexpr ULONGLONG SlowExtensionSliceIntervalMs = 167;
constexpr ULONGLONG SnapshotExtensionSliceIntervalMs = 503;
constexpr ULONGLONG VehicleExtensionTickIntervalMs = 500;
constexpr ULONGLONG ForgeSnapshotTickIntervalMs = 1000;
constexpr std::size_t RegularStatDrainBudget = 4;
constexpr GTA_Native_Hash SetRunSprintMultiplierHash = 0xA52E1AE3848A506BULL;
constexpr GTA_Native_Hash SetSwimMultiplierHash = 0x289497A4BA9049E0ULL;
constexpr GTA_Native_Hash SetPedMoveRateOverrideHash = 0xB27B08E34AC92345ULL;
constexpr float FastRunMoveRateOverride = 2.0F;

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

    const auto regionStart = reinterpret_cast<std::uintptr_t>(memory.BaseAddress);
    if (regionStart > (std::numeric_limits<std::uintptr_t>::max)() - memory.RegionSize)
        return false;
    const auto regionEnd = regionStart + memory.RegionSize;
    return address >= regionStart && address <= regionEnd && size <= regionEnd - address;
}

std::array<std::byte, GTA_Run_Script_Threads_Bridge::AbsoluteJumpSize> AbsoluteJump(
    std::uintptr_t destination) noexcept
{
    std::array<std::byte, GTA_Run_Script_Threads_Bridge::AbsoluteJumpSize> jump{};
    jump[0] = std::byte{0xFF};
    jump[1] = std::byte{0x25};
    jump[2] = std::byte{0x00};
    jump[3] = std::byte{0x00};
    jump[4] = std::byte{0x00};
    jump[5] = std::byte{0x00};
    std::memcpy(jump.data() + 6, &destination, sizeof(destination));
    return jump;
}

bool WriteExecutableBytes(
    std::uintptr_t address,
    const std::byte* bytes,
    std::size_t size) noexcept
{
    if (address == 0 || !bytes || size == 0)
        return false;

    DWORD oldProtection = 0;
    if (::VirtualProtect(reinterpret_cast<void*>(address), size, PAGE_EXECUTE_READWRITE, &oldProtection) == FALSE)
        return false;

    std::memcpy(reinterpret_cast<void*>(address), bytes, size);
    ::FlushInstructionCache(::GetCurrentProcess(), reinterpret_cast<const void*>(address), size);

    DWORD ignored = 0;
    return ::VirtualProtect(reinterpret_cast<void*>(address), size, oldProtection, &ignored) != FALSE;
}
}

GTA_Run_Script_Threads_Bridge* GTA_Run_Script_Threads_Bridge::s_active = nullptr;

GTA_Run_Script_Threads_Bridge::~GTA_Run_Script_Threads_Bridge()
{
    Uninstall();
}

bool GTA_Run_Script_Threads_Bridge::Install(
    std::uintptr_t runScriptThreadsAddress,
    std::uintptr_t scriptThreadsStorageAddress,
    std::uintptr_t expectedThreadDispatchAddress,
    GTA_Native_Manager& natives,
    Backend::LoggerService& logger) noexcept
{
    if (Installed())
        return true;

    if (runScriptThreadsAddress == 0 || scriptThreadsStorageAddress == 0 ||
        expectedThreadDispatchAddress == 0 || !natives.Ready() || s_active != nullptr) {
        return false;
    }

    if (!IsReadableAddress(runScriptThreadsAddress, PatchSize))
        return false;

    std::array<std::byte, PatchSize> current{};
    std::memcpy(current.data(), reinterpret_cast<const void*>(runScriptThreadsAddress), current.size());
    if (current != VerifiedPrologue)
        return false;

    constexpr std::size_t trampolineSize = PatchSize + AbsoluteJumpSize;
    auto* trampoline = ::VirtualAlloc(
        nullptr,
        trampolineSize,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE);
    if (!trampoline)
        return false;

    std::memcpy(trampoline, current.data(), current.size());
    const auto returnJump = AbsoluteJump(runScriptThreadsAddress + PatchSize);
    std::memcpy(
        static_cast<std::byte*>(trampoline) + PatchSize,
        returnJump.data(),
        returnJump.size());
    ::FlushInstructionCache(::GetCurrentProcess(), trampoline, trampolineSize);

    std::array<std::byte, PatchSize> patch{};
    const auto hookJump = AbsoluteJump(reinterpret_cast<std::uintptr_t>(&HookThunk));
    std::copy(hookJump.begin(), hookJump.end(), patch.begin());
    patch.back() = std::byte{0x90};

    m_targetAddress = runScriptThreadsAddress;
    m_scriptThreadsStorageAddress = scriptThreadsStorageAddress;
    m_expectedThreadDispatchAddress = expectedThreadDispatchAddress;
    m_originalBytes = current;
    m_trampoline = trampoline;
    m_original = reinterpret_cast<RunScriptThreads>(trampoline);
    m_natives = &natives;
    m_logger = &logger;
    ConfigureVehicleEditorLogging(&logger);
    ConfigureSelfOnlineExtensionLogging(&logger);
    m_smokeCompleted.store(false);
    m_smokeAttempting.store(false);
    m_activeCalls.store(0);
    m_cachedScriptThread.store(nullptr);
    const ULONGLONG scheduleBase = ::GetTickCount64();
    m_nextFeatureTickMs = scheduleBase;
    m_nextScheduledDispatchMs = scheduleBase + 8;
    m_nextGameplayTickMs = scheduleBase + 16;
    m_nextSelfUtilityTickMs = scheduleBase + 48;
    m_nextSelfCacheTickMs = scheduleBase + 84;
    m_nextOnlineExtensionTickMs = scheduleBase + 132;
    m_nextSlowExtensionTickMs = scheduleBase + 188;
    m_nextVehicleExtensionTickMs = scheduleBase + 244;
    m_nextSnapshotExtensionTickMs = scheduleBase + 356;
    m_nextForgeSnapshotTickMs = scheduleBase + 612;
    m_slowExtensionCursor = 0;
    m_snapshotExtensionCursor = 0;
    m_fastRunApplied = false;
    m_fastSwimApplied = false;
    m_gameplay.Configure(natives, logger);
    GTA_Gameplay_State::Instance().Reset();
    GTA_Self_Cache::Instance().Reset();
    ResetSelfUtilityExtension();
    ResetStatsExtension();
    ConfigureBunkerExtension(scriptThreadsStorageAddress, natives.Fingerprint());

    s_active = this;
    if (!WriteExecutableBytes(m_targetAddress, patch.data(), patch.size())) {
        s_active = nullptr;
        GTA_Self_Cache::Instance().Reset();
        ResetSelfUtilityExtension();
        ResetStatsExtension();
        ResetBunkerExtension();
        m_gameplay.Reset();
        ConfigureVehicleEditorLogging(nullptr);
        ConfigureSelfOnlineExtensionLogging(nullptr);
        m_original = nullptr;
        m_natives = nullptr;
        m_logger = nullptr;
        m_targetAddress = 0;
        m_scriptThreadsStorageAddress = 0;
        m_expectedThreadDispatchAddress = 0;
        m_trampoline = nullptr;
        ::VirtualFree(trampoline, 0, MEM_RELEASE);
        return false;
    }

    m_installed.store(true);
    logger.Log(
        Backend::LogLevel::Info,
        "RunScriptThreads bridge installed | Mode: validated direct game-thread feature tick",
        "GTA5_Enhanced.Natives");
    return true;
}

void GTA_Run_Script_Threads_Bridge::Uninstall() noexcept
{
    if (!m_installed.load())
        return;

    auto& gameplayState = GTA_Gameplay_State::Instance();
    gameplayState.SetGodMode(false);
    gameplayState.SetNeverWanted(false);

    ::Sleep(50);

    if (!m_installed.exchange(false))
        return;

    if (m_targetAddress != 0)
        WriteExecutableBytes(m_targetAddress, m_originalBytes.data(), m_originalBytes.size());

    while (m_activeCalls.load() != 0)
        ::Sleep(0);

    if (s_active == this)
        s_active = nullptr;

    GTA_Self_Cache::Instance().Reset();
    ResetSelfUtilityExtension();
    ResetStatsExtension();
    ResetBunkerExtension();
    ResetNetworkSessionExtension();
    ResetRandomEventsExtension();
    ResetSelfOnlineExtension();
    ConfigureVehicleEditorLogging(nullptr);
    ConfigureSelfOnlineExtensionLogging(nullptr);
    m_gameplay.Reset();
    gameplayState.Reset();
    m_original = nullptr;
    m_natives = nullptr;
    m_logger = nullptr;
    m_targetAddress = 0;
    m_scriptThreadsStorageAddress = 0;
    m_expectedThreadDispatchAddress = 0;
    m_cachedScriptThread.store(nullptr);
    m_nextFeatureTickMs = 0;
    m_nextScheduledDispatchMs = 0;
    m_nextGameplayTickMs = 0;
    m_nextSelfCacheTickMs = 0;
    m_nextSelfUtilityTickMs = 0;
    m_nextOnlineExtensionTickMs = 0;
    m_nextSlowExtensionTickMs = 0;
    m_nextSnapshotExtensionTickMs = 0;
    m_nextVehicleExtensionTickMs = 0;
    m_nextForgeSnapshotTickMs = 0;
    m_slowExtensionCursor = 0;
    m_snapshotExtensionCursor = 0;
    m_fastRunApplied = false;
    m_fastSwimApplied = false;

    if (m_trampoline) {
        ::VirtualFree(m_trampoline, 0, MEM_RELEASE);
        m_trampoline = nullptr;
    }
}

bool GTA_Run_Script_Threads_Bridge::HookThunk(int opsToExecute)
{
    auto* bridge = s_active;
    if (!bridge || !bridge->m_original)
        return false;

    bridge->m_activeCalls.fetch_add(1);
    const bool result = bridge->OnRunScriptThreads(opsToExecute);
    bridge->m_activeCalls.fetch_sub(1);
    return result;
}

bool GTA_Run_Script_Threads_Bridge::OnRunScriptThreads(int opsToExecute) noexcept
{
    const bool result = m_original ? m_original(opsToExecute) : false;
    if (m_installed.load(std::memory_order_relaxed) && m_natives)
        RunGameThreadFeatureTick();
    return result;
}

void GTA_Run_Script_Threads_Bridge::TryNativeSmoke() noexcept
{
    bool expected = false;
    if (!m_smokeAttempting.compare_exchange_strong(expected, true))
        return;

    if (!m_natives || !m_logger || !m_natives->Ready()) {
        m_smokeAttempting.store(false);
        return;
    }

    void* scriptThread = FindValidatedScriptThread();
    if (!scriptThread) {
        m_smokeAttempting.store(false);
        return;
    }

    const auto tlsArray = static_cast<std::uintptr_t>(__readgsqword(0x58));
    if (!IsReadableAddress(tlsArray, sizeof(void*))) {
        m_smokeAttempting.store(false);
        return;
    }

    GTA_Tls_Context_View* tls = nullptr;
    std::memcpy(&tls, reinterpret_cast<const void*>(tlsArray), sizeof(tls));
    if (!tls || !IsReadableAddress(reinterpret_cast<std::uintptr_t>(tls), sizeof(GTA_Tls_Context_View))) {
        m_smokeAttempting.store(false);
        return;
    }

    void* previousThread = tls->currentScriptThread;
    const bool previousActive = tls->scriptThreadActive;

    tls->currentScriptThread = scriptThread;
    tls->scriptThreadActive = true;
    const auto timer = m_natives->Invoke<int>(GTA_Native_Id::GetGameTimer);
    tls->scriptThreadActive = previousActive;
    tls->currentScriptThread = previousThread;

    if (timer) {
        m_smokeCompleted.store(true);
        m_logger->Log(
            Backend::LogLevel::Info,
            "GET_GAME_TIMER invocation succeeded | Result: " + std::to_string(*timer) +
                " | Execution: RunScriptThreads",
            "GTA5_Enhanced.Natives");
    }

    m_smokeAttempting.store(false);
}

void GTA_Run_Script_Threads_Bridge::RunGameThreadFeatureTick() noexcept
{
    if (!m_natives || !m_natives->Ready())
        return;

    const ULONGLONG now = ::GetTickCount64();
    auto& gameplayState = GTA_Gameplay_State::Instance();
    const auto personalSaveStatus = VehiclePersonalSaveStatus();
    const bool personalSaveActive =
        personalSaveStatus == GTA_Vehicle_Personal_Save_Status::Queued ||
        personalSaveStatus == GTA_Vehicle_Personal_Save_Status::Validating ||
        personalSaveStatus == GTA_Vehicle_Personal_Save_Status::OpeningGarageMenu ||
        personalSaveStatus == GTA_Vehicle_Personal_Save_Status::WaitingForGarageSelection;
    const bool pedControlActive = PedControlHasWork();
    const bool frameFeatureActive =
        gameplayState.SuperJump() ||
        gameplayState.FastRun() ||
        gameplayState.FastSwim() ||
        gameplayState.ExplosiveBullets() ||
        m_fastRunApplied ||
        m_fastSwimApplied ||
        personalSaveActive ||
        pedControlActive;
    const bool frameDue = frameFeatureActive && now >= m_nextFeatureTickMs;
    const bool scheduledWorkReady =
        HasPendingScriptFunctionInvocation() ||
        now >= m_nextGameplayTickMs ||
        now >= m_nextSelfCacheTickMs ||
        now >= m_nextSelfUtilityTickMs ||
        now >= m_nextOnlineExtensionTickMs ||
        now >= m_nextSlowExtensionTickMs ||
        now >= m_nextSnapshotExtensionTickMs ||
        now >= m_nextVehicleExtensionTickMs;
    const bool scheduledDue = scheduledWorkReady && now >= m_nextScheduledDispatchMs;
    if (!frameDue && !scheduledDue)
        return;

    void* scriptThread = FindValidatedScriptThread();
    if (!scriptThread)
        return;

    const auto tlsArray = static_cast<std::uintptr_t>(__readgsqword(0x58));
    if (!IsReadableAddress(tlsArray, sizeof(void*)))
        return;

    GTA_Tls_Context_View* tls = nullptr;
    std::memcpy(&tls, reinterpret_cast<const void*>(tlsArray), sizeof(tls));
    if (!tls || !IsReadableAddress(reinterpret_cast<std::uintptr_t>(tls), sizeof(GTA_Tls_Context_View)))
        return;

    void* previousThread = tls->currentScriptThread;
    const bool previousActive = tls->scriptThreadActive;

    tls->currentScriptThread = scriptThread;
    tls->scriptThreadActive = true;
    if (frameDue) {
        m_nextFeatureTickMs = now + FeatureTickIntervalMs;
        if (personalSaveActive)
            TickVehiclePersonalSave(*m_natives);
        if (pedControlActive)
            TickPedControl(*m_natives);
        m_gameplay.TickFrameSensitive();
        if (gameplayState.ExplosiveBullets())
            TickExplosiveAmmoExtension(*m_natives, scriptThread);
        TickFrameSensitiveMovement();
    }
    if (scheduledDue)
        RunLegacyGameplayTick(now);
    tls->scriptThreadActive = previousActive;
    tls->currentScriptThread = previousThread;
}

void GTA_Run_Script_Threads_Bridge::TickFrameSensitiveMovement() noexcept
{
    if (!m_natives || !m_natives->Ready())
        return;

    auto& state = GTA_Gameplay_State::Instance();
    const bool fastRun = state.FastRun();
    const bool fastSwim = state.FastSwim();
    if (!fastRun && !fastSwim && !m_fastRunApplied && !m_fastSwimApplied)
        return;

    const auto player = m_natives->Invoke<int>(GTA_Native_Id::PlayerId);
    if (!player)
        return;

    const float runMultiplier = fastRun ? state.RunSpeed() : 1.0F;
    const float swimMultiplier = fastSwim ? state.SwimSpeed() : 1.0F;
    (void)m_natives->InvokeDirectHash<void>(SetRunSprintMultiplierHash, *player, runMultiplier);
    (void)m_natives->InvokeDirectHash<void>(SetSwimMultiplierHash, *player, swimMultiplier);

    if (fastRun) {
        const auto ped = m_natives->Invoke<int>(GTA_Native_Id::PlayerPedId);
        if (ped && *ped != 0)
            (void)m_natives->InvokeDirectHash<void>(SetPedMoveRateOverrideHash, *ped, FastRunMoveRateOverride);
    }

    m_fastRunApplied = fastRun;
    m_fastSwimApplied = fastSwim;
}

void GTA_Run_Script_Threads_Bridge::RunLegacyGameplayTick(std::uint64_t now) noexcept
{
    if (!m_natives || !m_natives->Ready())
        return;

    m_nextScheduledDispatchMs = now + ScheduledDispatchGapMs;

    if (HasPendingScriptFunctionInvocation()) {
        (void)TickScriptFunctionInvoker();
        return;
    }

    if (now >= m_nextGameplayTickMs) {
        auto& gameplayState = GTA_Gameplay_State::Instance();
        auto& vehicleState = GTA_Vehicle_State::Instance();
        const auto spawnStatus = vehicleState.SpawnStatus();
        const auto teleportStatus = gameplayState.TeleportStatus();
        const bool gameplayHot =
            gameplayState.MenuInputCaptured() ||
            gameplayState.GodMode() ||
            gameplayState.NeverWanted() ||
            gameplayState.InfiniteOxygen() ||
            gameplayState.NoRagdoll() ||
            gameplayState.KeepPlayerClean() ||
            gameplayState.InfiniteAmmo() ||
            gameplayState.UnlimitedClip() ||
            vehicleState.KeepVehiclePerfect() ||
            vehicleState.VehicleGodMode() ||
            spawnStatus == GTA_Vehicle_Spawn_Status::Queued ||
            spawnStatus == GTA_Vehicle_Spawn_Status::Validating ||
            spawnStatus == GTA_Vehicle_Spawn_Status::Streaming ||
            spawnStatus == GTA_Vehicle_Spawn_Status::Creating ||
            spawnStatus == GTA_Vehicle_Spawn_Status::Applying ||
            teleportStatus == GTA_Teleport_Waypoint_Status::Queued ||
            teleportStatus == GTA_Teleport_Waypoint_Status::Resolving;

        m_nextGameplayTickMs = now +
            (gameplayHot ? GameplayTickIntervalMs : IdleGameplayTickIntervalMs);

        const bool explosiveAmmo = gameplayState.ExplosiveBullets();
        if (explosiveAmmo)
            gameplayState.SetExplosiveBullets(false);
        m_gameplay.Tick();
        if (now >= m_nextForgeSnapshotTickMs) {
            m_nextForgeSnapshotTickMs = now + ForgeSnapshotTickIntervalMs;
            m_gameplay.TickSlow();
        }
        (void)TickStatsExtension(*m_natives, RegularStatDrainBudget);
        if (explosiveAmmo)
            gameplayState.SetExplosiveBullets(true);
        return;
    }

    if (now >= m_nextSelfUtilityTickMs) {
        m_nextSelfUtilityTickMs = now + SelfUtilityTickIntervalMs;
        TickSelfUtilityExtension(*m_natives);
        return;
    }

    if (now >= m_nextSelfCacheTickMs) {
        m_nextSelfCacheTickMs = now + SelfCacheTickIntervalMs;
        GTA_Self_Cache::Instance().Update(*m_natives);
        return;
    }

    if (now >= m_nextOnlineExtensionTickMs) {
        m_nextOnlineExtensionTickMs = now + OnlineExtensionTickIntervalMs;
        TickSelfOnlineExtension(*m_natives);
        return;
    }

    if (now >= m_nextSlowExtensionTickMs) {
        m_nextSlowExtensionTickMs = now + SlowExtensionSliceIntervalMs;
        switch (m_slowExtensionCursor) {
        case 0:
            TickNetworkSessionExtension();
            break;
        case 1:
            TickRandomEventsExtension(*m_natives);
            break;
        default:
            TickBunkerExtension();
            break;
        }
        m_slowExtensionCursor = static_cast<std::uint8_t>((m_slowExtensionCursor + 1U) % 3U);
        return;
    }

    if (now >= m_nextVehicleExtensionTickMs) {
        m_nextVehicleExtensionTickMs = now + VehicleExtensionTickIntervalMs;
        TickVehicleEditorExtensions(*m_natives);
        return;
    }

    if (now >= m_nextSnapshotExtensionTickMs) {
        m_nextSnapshotExtensionTickMs = now + SnapshotExtensionSliceIntervalMs;
        if (m_snapshotExtensionCursor == 0)
            TickBusinessExtension(*m_natives);
        else
            TickCasinoExtension(*m_natives);
        m_snapshotExtensionCursor ^= 1U;
        return;
    }
}

void* GTA_Run_Script_Threads_Bridge::FindValidatedScriptThread() const noexcept
{
    if (void* cached = m_cachedScriptThread.load(std::memory_order_relaxed)) {
        if (IsReadableAddress(reinterpret_cast<std::uintptr_t>(cached), sizeof(std::uintptr_t))) {
            std::uintptr_t dispatch = 0;
            std::memcpy(&dispatch, cached, sizeof(dispatch));
            if (dispatch == m_expectedThreadDispatchAddress)
                return cached;
        }
        m_cachedScriptThread.store(nullptr, std::memory_order_relaxed);
    }

    if (!IsReadableAddress(m_scriptThreadsStorageAddress, sizeof(std::uintptr_t)))
        return nullptr;

    std::uintptr_t slotsAddress = 0;
    std::memcpy(
        &slotsAddress,
        reinterpret_cast<const void*>(m_scriptThreadsStorageAddress),
        sizeof(slotsAddress));

    constexpr std::size_t slotCount = 8;
    if (!IsReadableAddress(slotsAddress, slotCount * sizeof(std::uintptr_t)))
        return nullptr;

    for (std::size_t i = 0; i < slotCount; ++i) {
        std::uintptr_t candidate = 0;
        std::memcpy(
            &candidate,
            reinterpret_cast<const void*>(slotsAddress + (i * sizeof(std::uintptr_t))),
            sizeof(candidate));

        if (!IsReadableAddress(candidate, sizeof(std::uintptr_t)))
            continue;

        std::uintptr_t dispatch = 0;
        std::memcpy(&dispatch, reinterpret_cast<const void*>(candidate), sizeof(dispatch));
        if (dispatch == m_expectedThreadDispatchAddress) {
            void* resolved = reinterpret_cast<void*>(candidate);
            m_cachedScriptThread.store(resolved, std::memory_order_relaxed);
            return resolved;
        }
    }

    return nullptr;
}
}