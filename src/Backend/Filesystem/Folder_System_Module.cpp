#include "Folder_System_Module.hpp"

#include <utility>

namespace Devilz::Backend
{
Folder_System_Module::Folder_System_Module(std::filesystem::path root)
    : m_folders(std::move(root))
{
}

Result<void> Folder_System_Module::Initialize()
{
    m_state.store(ServiceState::Initializing);

    const std::pair<const char*, const char*> defaults[] = {
        {"Logs", "Logs"},
        {"Configs", "Configs"},
        {"Scripts", "Scripts"},
        {"Cache", "Cache"},
        {"CrashReports", "CrashReports"},
        {"Resources", "Resources"},
        {"Temp", "Temp"}
    };

    for (const auto& [name, path] : defaults) {
        auto result = m_folders.RegisterFolder(name, path, false);
        if (!result) {
            m_state.store(ServiceState::Failed);
            return result;
        }
    }

    auto ensured = m_folders.EnsureRegisteredFolders();
    if (!ensured) {
        m_state.store(ServiceState::Failed);
        return ensured;
    }

    m_state.store(ServiceState::Created);
    return Result<void>::Success();
}

Result<void> Folder_System_Module::Start()
{
    const auto state = m_state.load();
    if (state == ServiceState::Running)
        return Result<void>::Success();
    if (state == ServiceState::Failed)
        return Result<void>::Failure(Error(ErrorCode::ServiceInitializationFailed, ErrorCategory::Service, "Folder system cannot start from failed state"));

    m_state.store(ServiceState::Running);
    return Result<void>::Success();
}

Result<void> Folder_System_Module::Stop()
{
    m_state.store(ServiceState::Stopping);
    m_monitors.StopAll();
    m_state.store(ServiceState::Stopped);
    return Result<void>::Success();
}

void Folder_System_Module::Shutdown() noexcept
{
    m_monitors.StopAll();
    m_state.store(ServiceState::Stopped);
}
}
