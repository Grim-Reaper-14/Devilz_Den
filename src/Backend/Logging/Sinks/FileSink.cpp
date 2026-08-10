#include "FileSink.hpp"

#include <chrono>
#include <iomanip>

namespace Devilz::Backend
{
namespace
{
const char* LevelName(LogLevel level) noexcept
{
    switch (level) {
    case LogLevel::Trace: return "TRACE";
    case LogLevel::Debug: return "DEBUG";
    case LogLevel::Info: return "INFO";
    case LogLevel::Notice: return "NOTICE";
    case LogLevel::Warning: return "WARN";
    case LogLevel::Error: return "ERROR";
    case LogLevel::Critical: return "CRIT";
    case LogLevel::Fatal: return "FATAL";
    default: return "UNKNOWN";
    }
}
}

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
    const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                                  record.timestamp.time_since_epoch()) %
                              std::chrono::seconds(1);

    std::tm local{};
    localtime_s(&local, &time);

    m_stream << std::put_time(&local, "%Y-%m-%d %H:%M:%S")
             << '.' << std::setfill('0') << std::setw(3) << milliseconds.count()
             << " | #" << std::setw(6) << record.sequence
             << " | " << std::left << std::setfill(' ') << std::setw(6) << LevelName(record.level)
             << " | " << std::left << std::setw(24) << record.service
             << " | " << record.message;

    if (record.error)
        m_stream << " | " << record.error->DetailedDescription();

    m_stream << '\n';
    m_stream.flush();
}

void FileSink::Flush()
{
    std::scoped_lock lock(m_mutex);
    if (m_stream) m_stream.flush();
}
}
