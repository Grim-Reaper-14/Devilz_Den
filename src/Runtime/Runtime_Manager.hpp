#pragma once

#include "Backend/Logging/LoggerService.hpp"
#include "Backend/Threading/ThreadManager.hpp"
#include "Frontend/Frontend_Manager.hpp"
#include "Integrations/GTA5_Enhanced/GTA_Module_Manager.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Manager.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Run_Script_Threads_Bridge.hpp"
#include "Integrations/GTA5_Enhanced/Script/Globals/Script_Global_Manager.hpp"

#include <atomic>
#include <filesystem>

namespace Devilz
{
class Runtime_Manager final
{
public:
    Runtime_Manager() = default;
    ~Runtime_Manager();

    Runtime_Manager(const Runtime_Manager&) = delete;
    Runtime_Manager& operator=(const Runtime_Manager&) = delete;

    [[nodiscard]] bool Start(const std::filesystem::path& logPath);
    void Stop() noexcept;

    [[nodiscard]] bool Running() const noexcept { return m_running.load(); }

private:
    void LogGTAStatus(const Integrations::GTA5_Enhanced::GTA_Module_Status& status);
    void InitializeNativeManager(const Integrations::GTA5_Enhanced::GTA_Module_Status& status);
    void InitializeGameThreadBridge(const Integrations::GTA5_Enhanced::GTA_Module_Status& status);

    Backend::LoggerService m_logger;
    Backend::ThreadManager m_threads;
    Integrations::GTA5_Enhanced::GTA_Module_Manager m_gta;
    Integrations::GTA5_Enhanced::GTA_Native_Manager m_natives;
    Integrations::GTA5_Enhanced::Script_Global_Manager m_scriptGlobals;
    Integrations::GTA5_Enhanced::GTA_Run_Script_Threads_Bridge m_gameThreadBridge;
    Frontend::Frontend_Manager m_frontend;
    std::atomic_bool m_running{false};
};
}
