#pragma once

#include "Backend/Logging/LoggerService.hpp"
#include "Backend/Threading/ThreadManager.hpp"
#include "Integrations/GTA5_Enhanced/GTA_Module_Manager.hpp"

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

    Backend::LoggerService m_logger;
    Backend::ThreadManager m_threads;
    Integrations::GTA5_Enhanced::GTA_Module_Manager m_gta;
    std::atomic_bool m_running{false};
};
}
