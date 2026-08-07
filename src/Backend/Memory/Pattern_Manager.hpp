#pragma once

#include "Backend/Filesystem/File_System_Manager.hpp"
#include "Backend/Services/IBackendService.hpp"
#include "Backend/Threading/ThreadManager.hpp"
#include "Module_Manager.hpp"
#include "Pattern_Batch_Scanner.hpp"
#include "Pattern_Cache_Persistence.hpp"

#include <atomic>
#include <filesystem>
#include <string_view>
#include <vector>

namespace Devilz::Backend
{
class Pattern_Manager final : public IBackendService
{
public:
    Pattern_Manager(ThreadManager& threads,
                    File_System_Manager& files,
                    std::filesystem::path cachePath = "Cache/patterns.ddpc");

    [[nodiscard]] std::string_view Name() const noexcept override { return "Pattern_Manager"; }
    [[nodiscard]] ServiceState State() const noexcept override { return m_state.load(std::memory_order_acquire); }

    Result<void> Initialize() override;
    Result<void> Start() override;
    Result<void> Stop() override;
    void Shutdown() noexcept override;

    [[nodiscard]] Result<std::vector<Pattern_Resolution>> ResolveMain(
        const std::vector<Pattern_Request>& requests);
    [[nodiscard]] Result<std::vector<Pattern_Resolution>> ResolveModule(
        std::string_view moduleName,
        const std::vector<Pattern_Request>& requests);

    [[nodiscard]] Pattern_Cache& Cache() noexcept { return m_cache; }
    [[nodiscard]] Module_Manager& Modules() noexcept { return m_modules; }

private:
    [[nodiscard]] Result<std::vector<Pattern_Resolution>> ResolveParallel(
        const Module_Info& module,
        const std::vector<Pattern_Request>& requests);
    Result<void> PersistCache();

    ThreadManager& m_threads;
    File_System_Manager& m_files;
    std::filesystem::path m_cachePath;
    Module_Manager m_modules;
    Pattern_Cache m_cache;
    Pattern_Cache_Persistence m_persistence;
    std::atomic<ServiceState> m_state{ServiceState::Created};
};
}
