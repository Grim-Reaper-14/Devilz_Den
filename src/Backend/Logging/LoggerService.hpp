#pragma once

#include "ILogSink.hpp"

#include <atomic>
#include <condition_variable>
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

private:
    void Worker(std::stop_token stopToken);
    void Dispatch(const LogRecord& record);

    std::atomic<std::uint64_t> m_sequence{0};
    std::mutex m_queueMutex;
    std::mutex m_sinkMutex;
    std::condition_variable_any m_cv;
    std::deque<LogRecord> m_queue;
    std::vector<std::unique_ptr<ILogSink>> m_sinks;
    std::jthread m_worker;
    bool m_accepting = false;
};
}
