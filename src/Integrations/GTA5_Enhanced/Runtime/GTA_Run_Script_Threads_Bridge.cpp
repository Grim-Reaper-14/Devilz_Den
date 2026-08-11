#include "GTA_Run_Script_Threads_Bridge.hpp"

#include "GTA_Explosive_Ammo_Extension.hpp"
#include "GTA_Gameplay_State.hpp"
#include "GTA_Network_Session_Extension.hpp"
#include "GTA_Vehicle_Forge_Extensions.hpp"
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
    m_smokeCompleted.store(false);
    m_smokeAttempting.store(false);
    m_activeCalls.store(0);
    m_gameplay.Configure(natives, logger);
    GTA_Gameplay_State::Instance().Reset();

    s_active = this;
    if (!WriteExecutableBytes(m_targetAddress, patch.data(), patch.size())) {
        s_active = nullptr;
        m_gameplay.Reset();
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
        "RunScriptThreads bridge installed | Mode: native smoke + gameplay features",
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

    ResetNetworkSessionExtension();
    m_gameplay.Reset();
    gameplayState.Reset();
    m_original = nullptr;
    m_natives = nullptr;
    m_logger = nullptr;
    m_targetAddress = 0;
    m_scriptThreadsStorageAddress = 0;
    m_expectedThreadDispatchAddress = 0;

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
    if (!m_installed.load())
        return result;

    if (!m_smokeCompleted.load())
        TryNativeSmoke();

    RunGameplayTick();
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

void GTA_Run_Script_Threads_Bridge::RunGameplayTick() noexcept
{
    if (!m_natives || !m_natives->Ready())
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

    TickNetworkSessionExtension();

    // Explosive ammo owns its dedicated Yim-style spoofed extension path. Keep
    // the legacy gameplay runner from emitting an unspoofed duplicate.
    auto& gameplayState = GTA_Gameplay_State::Instance();
    const bool explosiveAmmo = gameplayState.ExplosiveBullets();
    if (explosiveAmmo)
        gameplayState.SetExplosiveBullets(false);
    m_gameplay.Tick();
    if (explosiveAmmo)
        gameplayState.SetExplosiveBullets(true);

    TickExplosiveAmmoExtension(*m_natives, scriptThread);

    // The vehicle extension no longer receives a script-thread identity here;
    // this keeps its legacy explosive-ammo helper dormant while preserving the
    // vehicle/self extension work it also performs.
    TickVehicleForgeExtensions(*m_natives);

    tls->scriptThreadActive = previousActive;
    tls->currentScriptThread = previousThread;
}

void* GTA_Run_Script_Threads_Bridge::FindValidatedScriptThread() const noexcept
{
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
        if (dispatch == m_expectedThreadDispatchAddress)
            return reinterpret_cast<void*>(candidate);
    }

    return nullptr;
}
}
