#pragma once

#include "File_System_Monitor.hpp"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace Devilz::Backend
{
class File_System_Monitor_Manager final
{
public:
    Result<void> Add(std::string name,
                     std::filesystem::path root,
                     File_System_Monitor::Callback callback,
                     std::chrono::milliseconds interval = std::chrono::milliseconds{250},
                     bool recursive = true);

    Result<void> Start(std::string_view name);
    Result<void> Remove(std::string_view name);
    void StopAll() noexcept;

    [[nodiscard]] File_System_Monitor* Find(std::string_view name) noexcept;
    [[nodiscard]] std::size_t Count() const noexcept;

private:
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, std::unique_ptr<File_System_Monitor>> m_monitors;
};
}
