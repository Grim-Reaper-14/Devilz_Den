#include "File_System_Monitor_Manager.hpp"

namespace Devilz::Backend
{
Result<void> File_System_Monitor_Manager::Add(std::string name, std::filesystem::path root,
                                              File_System_Monitor::Callback callback,
                                              std::chrono::milliseconds interval, bool recursive)
{
    if (name.empty())
        return Result<void>::Failure(Error(ErrorCode::DirectoryOpenFailed, ErrorCategory::Filesystem, "Monitor requires a name"));

    std::scoped_lock lock(m_mutex);
    if (m_monitors.contains(name))
        return Result<void>::Failure(Error(ErrorCode::DirectoryOpenFailed, ErrorCategory::Filesystem, "Filesystem monitor already exists").With("Monitor", name));

    m_monitors.emplace(std::move(name), std::make_unique<File_System_Monitor>(std::move(root), std::move(callback), interval, recursive));
    return Result<void>::Success();
}

Result<void> File_System_Monitor_Manager::Start(std::string_view name)
{
    std::scoped_lock lock(m_mutex);
    const auto it = m_monitors.find(std::string(name));
    if (it == m_monitors.end())
        return Result<void>::Failure(Error(ErrorCode::DirectoryOpenFailed, ErrorCategory::Filesystem, "Filesystem monitor not found").With("Monitor", std::string(name)));
    return it->second->Start();
}

Result<void> File_System_Monitor_Manager::Remove(std::string_view name)
{
    std::unique_ptr<File_System_Monitor> monitor;
    {
        std::scoped_lock lock(m_mutex);
        const auto it = m_monitors.find(std::string(name));
        if (it == m_monitors.end())
            return Result<void>::Failure(Error(ErrorCode::DirectoryOpenFailed, ErrorCategory::Filesystem, "Filesystem monitor not found").With("Monitor", std::string(name)));
        monitor = std::move(it->second);
        m_monitors.erase(it);
    }
    monitor->Stop();
    return Result<void>::Success();
}

void File_System_Monitor_Manager::StopAll() noexcept
{
    std::vector<File_System_Monitor*> monitors;
    {
        std::scoped_lock lock(m_mutex);
        monitors.reserve(m_monitors.size());
        for (auto& [name, monitor] : m_monitors) {
            (void)name;
            monitors.push_back(monitor.get());
        }
    }
    for (auto* monitor : monitors) monitor->Stop();
}

File_System_Monitor* File_System_Monitor_Manager::Find(std::string_view name) noexcept
{
    std::scoped_lock lock(m_mutex);
    const auto it = m_monitors.find(std::string(name));
    return it == m_monitors.end() ? nullptr : it->second.get();
}

std::size_t File_System_Monitor_Manager::Count() const noexcept
{
    std::scoped_lock lock(m_mutex);
    return m_monitors.size();
}
}
