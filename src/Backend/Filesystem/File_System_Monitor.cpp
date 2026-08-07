#include "File_System_Monitor.hpp"

#include "Backend/Logging/Logger_Manager.hpp"

namespace Devilz::Backend
{
File_System_Monitor::File_System_Monitor(std::filesystem::path root, Callback callback,
                                         std::chrono::milliseconds interval, bool recursive)
    : m_root(std::move(root)),
      m_callback(std::move(callback)),
      m_interval(interval),
      m_recursive(recursive)
{
}

File_System_Monitor::~File_System_Monitor()
{
    Stop();
}

Result<void> File_System_Monitor::Start()
{
    if (m_running.exchange(true))
        return Result<void>::Success();

    auto snapshot = BuildSnapshot();
    if (!snapshot) {
        m_running.store(false);
        return Result<void>::Failure(snapshot.Failure());
    }

    m_snapshot = std::move(snapshot.Value());
    m_worker = std::jthread([this](std::stop_token token) { Worker(token); });
    return Result<void>::Success();
}

void File_System_Monitor::Stop() noexcept
{
    if (!m_running.exchange(false))
        return;
    if (m_worker.joinable()) {
        m_worker.request_stop();
        m_worker.join();
    }
}

Result<File_System_Monitor::Snapshot> File_System_Monitor::BuildSnapshot() const
{
    Snapshot snapshot;
    std::error_code ec;

    if (!std::filesystem::is_directory(m_root, ec) || ec) {
        return Result<Snapshot>::Failure(
            Error::FromWin32(ErrorCode::DirectoryOpenFailed, ErrorCategory::Filesystem,
                             static_cast<std::uint32_t>(ec.value()), "Unable to monitor directory")
                .With("Path", m_root.string())
                .With("SystemMessage", ec.message()));
    }

    auto add = [&](const std::filesystem::directory_entry& entry) {
        std::error_code timeError;
        const auto time = entry.last_write_time(timeError);
        if (!timeError)
            snapshot.insert_or_assign(entry.path(), time);
    };

    if (m_recursive) {
        std::filesystem::recursive_directory_iterator it(m_root, std::filesystem::directory_options::skip_permission_denied, ec), end;
        for (; !ec && it != end; it.increment(ec)) add(*it);
    } else {
        std::filesystem::directory_iterator it(m_root, std::filesystem::directory_options::skip_permission_denied, ec), end;
        for (; !ec && it != end; it.increment(ec)) add(*it);
    }

    if (ec)
        return Result<Snapshot>::Failure(Error::FromWin32(ErrorCode::DirectoryOpenFailed, ErrorCategory::Filesystem,
            static_cast<std::uint32_t>(ec.value()), "Filesystem snapshot failed").With("Path", m_root.string()).With("SystemMessage", ec.message()));

    return Result<Snapshot>::Success(std::move(snapshot));
}

void File_System_Monitor::Worker(std::stop_token token)
{
    while (!token.stop_requested() && m_running.load()) {
        std::this_thread::sleep_for(m_interval);
        if (token.stop_requested()) break;

        auto currentResult = BuildSnapshot();
        if (!currentResult) {
            Logger_Manager::Instance().LogError(LogLevel::Error, currentResult.Failure(), "File_System_Monitor");
            continue;
        }

        auto current = std::move(currentResult.Value());
        for (const auto& [path, time] : current) {
            const auto previous = m_snapshot.find(path);
            if (previous == m_snapshot.end()) {
                if (m_callback) m_callback({File_System_Event_Type::Created, path});
            } else if (previous->second != time) {
                if (m_callback) m_callback({File_System_Event_Type::Modified, path});
            }
        }

        for (const auto& [path, unused] : m_snapshot) {
            (void)unused;
            if (!current.contains(path) && m_callback)
                m_callback({File_System_Event_Type::Removed, path});
        }

        m_snapshot = std::move(current);
    }
}
}
