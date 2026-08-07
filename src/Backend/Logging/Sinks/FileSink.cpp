#include "FileSink.hpp"

#include <chrono>
#include <iomanip>

namespace Devilz::Backend
{
FileSink::FileSink(const std::filesystem::path& path)
{
    if (path.has_parent_path())
        std::filesystem::create_directories(path.parent_path());
    m_stream.open(path, std::ios::out | std::ios::app);
}

void FileSink::Write(const LogRecord& record)
{
    std::scoped_lock lock(m_mutex);
    if (!m_stream) return;

    const auto time = std::chrono::system_clock::to_time_t(record.timestamp);
    std::tm local{};
    localtime_s(&local, &time);

    m_stream << std::put_time(&local, "%Y-%m-%d %H:%M:%S")
             << " [" << record.sequence << "] [" << record.service << "] " << record.message;
    if (record.error) m_stream << " | " << record.error->DetailedDescription();
    m_stream << '\n';
}

void FileSink::Flush()
{
    std::scoped_lock lock(m_mutex);
    if (m_stream) m_stream.flush();
}
}
