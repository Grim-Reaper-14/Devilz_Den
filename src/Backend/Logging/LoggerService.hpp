#pragma once

#include "ILogSink.hpp"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <memory>
#include <mutex>
#include <source_location>
#include <string>
#include <thread>
#include <vector>

namespace Devilz::Backend
{
class LoggerService
{
public:
    static constexpr std::size_t MaxQueuedRecords = 4096U;

    LoggerService() = default;
    ~LoggerService();

    LoggerService(const LoggerService&) = delete;
    LoggerService& operator=(const LoggerService&) = delete;

    void AddSink(std::unique_ptr<ILogSink> sink);
    void Start();
    void Stop();
    void Flush();

    void Log(LogLevel level, std::string message, std::string service = {},
             std::source_location source = std::source_location::current());
    void LogError(LogLevel level, const Error& error, std::string service = {},
                  std::source_location source = std::source_location::current());
    void LogWithContext(LogLevel level, std::string message, LogContext context,
                        std::source_location source = std::source_location::current());
    void LogErrorWithContext(LogLevel level, const Error& error, LogContext context,
                             std::source_location source = std::source_location::current());

    [[nodiscard]] std::size_t PendingRecordCount() const noexcept;
    [[nodiscard]] std::uint64_t DroppedRecordCount() const noexcept
    {
        return m_droppedRecords.load(std::memory_order_relaxed);
    }

private:
    bool Enqueue(LogRecord record);
    void Worker(std::stop_token stopToken);
    void Dispatch(const LogRecord& record);

    std::atomic<std::uint64_t> m_sequence{0};
    std::atomic<std::uint64_t> m_droppedRecords{0};
    mutable std::mutex m_queueMutex;
    std::mutex m_sinkMutex;
    std::condition_variable_any m_cv;
    std::deque<LogRecord> m_queue;
    std::vector<std::unique_ptr<ILogSink>> m_sinks;
    std::jthread m_worker;
    bool m_accepting = false;
};
}
