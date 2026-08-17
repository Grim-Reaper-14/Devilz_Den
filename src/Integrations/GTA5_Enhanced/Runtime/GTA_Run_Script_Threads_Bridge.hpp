#pragma once

#include "Backend/Logging/LoggerService.hpp"
#include "GTA_Gameplay_Feature_Runner.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Manager.hpp"

#include <Windows.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace Devilz::Integrations::GTA5_Enhanced
{
class GTA_Run_Script_Threads_Bridge final
{
public:
    using RunScriptThreads = bool (*)(int);

    static constexpr std::size_t PatchSize = 15;
    static constexpr std::size_t AbsoluteJumpSize = 14;

    GTA_Run_Script_Threads_Bridge() = default;
    ~GTA_Run_Script_Threads_Bridge();

    GTA_Run_Script_Threads_Bridge(const GTA_Run_Script_Threads_Bridge&) = delete;
    GTA_Run_Script_Threads_Bridge& operator=(const GTA_Run_Script_Threads_Bridge&) = delete;

    [[nodiscard]] bool Install(
        std::uintptr_t runScriptThreadsAddress,
        std::uintptr_t scriptThreadsStorageAddress,
        std::uintptr_t expectedThreadDispatchAddress,
        GTA_Native_Manager& natives,
        Backend::LoggerService& logger) noexcept;

    void Uninstall() noexcept;

    [[nodiscard]] bool Installed() const noexcept { return m_installed.load(); }
    [[nodiscard]] bool SmokeCompleted() const noexcept { return m_smokeCompleted.load(); }

private:
    static bool HookThunk(int opsToExecute);
    bool OnRunScriptThreads(int opsToExecute) noexcept;
    void TryNativeSmoke() noexcept;
    void RunGameThreadFeatureTick() noexcept;
    void RunLegacyGameplayTick(std::uint64_t now) noexcept;
    void TickFrameSensitiveMovement() noexcept;
    [[nodiscard]] void* FindValidatedScriptThread() const noexcept;

    std::atomic_bool m_installed{false};
    std::atomic_bool m_smokeCompleted{false};
    std::atomic_bool m_smokeAttempting{false};
    std::atomic_uint32_t m_activeCalls{0};
    mutable std::atomic<void*> m_cachedScriptThread{nullptr};

    std::uintptr_t m_targetAddress = 0;
    std::uintptr_t m_scriptThreadsStorageAddress = 0;
    std::uintptr_t m_expectedThreadDispatchAddress = 0;
    std::uint64_t m_nextFeatureTickMs = 0;
    std::uint64_t m_nextScheduledDispatchMs = 0;
    std::uint64_t m_nextGameplayTickMs = 0;
    std::uint64_t m_nextSelfCacheTickMs = 0;
    std::uint64_t m_nextSelfUtilityTickMs = 0;
    std::uint64_t m_nextOnlineExtensionTickMs = 0;
    std::uint64_t m_nextSlowExtensionTickMs = 0;
    std::uint64_t m_nextSnapshotExtensionTickMs = 0;
    std::uint64_t m_nextVehicleExtensionTickMs = 0;
    std::uint64_t m_nextForgeSnapshotTickMs = 0;
    std::uint64_t m_nextWorldEnvironmentTickMs = 0;
    std::uint8_t m_slowExtensionCursor = 0;
    std::uint8_t m_snapshotExtensionCursor = 0;
    bool m_fastRunApplied = false;
    bool m_fastSwimApplied = false;
    std::array<std::byte, PatchSize> m_originalBytes{};
    void* m_trampoline = nullptr;
    RunScriptThreads m_original = nullptr;
    GTA_Native_Manager* m_natives = nullptr;
    Backend::LoggerService* m_logger = nullptr;
    GTA_Gameplay_Feature_Runner m_gameplay;

    static GTA_Run_Script_Threads_Bridge* s_active;
};
}
