#pragma once

#include "Backend/Error/Result.hpp"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

namespace Devilz::Backend
{
enum class File_System_Event_Type
{
    Created,
    Modified,
    Removed
};

struct File_System_Event
{
    File_System_Event_Type type{};
    std::filesystem::path path;
};

class File_System_Monitor final
{
public:
    using Callback = std::function<void(const File_System_Event&)>;

    File_System_Monitor(std::filesystem::path root,
                        Callback callback,
                        std::chrono::milliseconds interval = std::chrono::milliseconds{250},
                        bool recursive = true);
    ~File_System_Monitor();

    File_System_Monitor(const File_System_Monitor&) = delete;
    File_System_Monitor& operator=(const File_System_Monitor&) = delete;

    Result<void> Start();
    void Stop() noexcept;
    [[nodiscard]] bool IsRunning() const noexcept { return m_running.load(); }
    [[nodiscard]] const std::filesystem::path& Root() const noexcept { return m_root; }

private:
    using Snapshot = std::unordered_map<std::filesystem::path, std::filesystem::file_time_type>;

    Result<Snapshot> BuildSnapshot() const;
    void Worker(std::stop_token token);

    std::filesystem::path m_root;
    Callback m_callback;
    std::chrono::milliseconds m_interval;
    bool m_recursive = true;
    std::atomic_bool m_running{false};
    std::jthread m_worker;
    Snapshot m_snapshot;
};
}
