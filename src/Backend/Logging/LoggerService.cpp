#include "LoggerService.hpp"

#include <utility>

namespace Devilz::Backend
{
LoggerService::~LoggerService()
{
    Stop();
}

void LoggerService::AddSink(std::unique_ptr<ILogSink> sink)
{
    std::scoped_lock lock(m_sinkMutex);
    m_sinks.push_back(std::move(sink));
}

void LoggerService::Start()
{
    std::scoped_lock lock(m_queueMutex);
    if (m_accepting) return;
    m_accepting = true;
    m_worker = std::jthread([this](std::stop_token token) { Worker(token); });
}

void LoggerService::Stop()
{
    {
        std::scoped_lock lock(m_queueMutex);
        if (!m_accepting && !m_worker.joinable()) return;
        m_accepting = false;
    }
    m_cv.notify_all();
    if (m_worker.joinable()) {
        m_worker.request_stop();
        m_worker.join();
    }
    Flush();
}

void LoggerService::Flush()
{
    std::scoped_lock lock(m_sinkMutex);
    for (auto& sink : m_sinks) sink->Flush();
}

void LoggerService::Log(LogLevel level, std::string message, std::string service, std::source_location source)
{
    LogRecord record;
    record.sequence = ++m_sequence;
    record.level = level;
    record.timestamp = std::chrono::system_clock::now();
    record.message = std::move(message);
    record.service = std::move(service);
    record.threadId = std::this_thread::get_id();
    record.source = source;

    {
        std::scoped_lock lock(m_queueMutex);
        if (!m_accepting) return;
        m_queue.push_back(std::move(record));
    }
    m_cv.notify_one();
}

void LoggerService::LogError(LogLevel level, const Error& error, std::string service, std::source_location source)
{
    LogRecord record;
    record.sequence = ++m_sequence;
    record.level = level;
    record.timestamp = std::chrono::system_clock::now();
    record.message = error.Message();
    record.service = std::move(service);
    record.threadId = std::this_thread::get_id();
    record.source = source;
    record.error = error;

    {
        std::scoped_lock lock(m_queueMutex);
        if (!m_accepting) return;
        m_queue.push_back(std::move(record));
    }
    m_cv.notify_one();
}

void LoggerService::Worker(std::stop_token stopToken)
{
    for (;;) {
        LogRecord record;
        {
            std::unique_lock lock(m_queueMutex);
            m_cv.wait(lock, stopToken, [this] { return !m_queue.empty() || !m_accepting; });
            if (m_queue.empty()) {
                if (stopToken.stop_requested() || !m_accepting) break;
                continue;
            }
            record = std::move(m_queue.front());
            m_queue.pop_front();
        }
        Dispatch(record);
    }

    for (;;) {
        LogRecord record;
        {
            std::scoped_lock lock(m_queueMutex);
            if (m_queue.empty()) break;
            record = std::move(m_queue.front());
            m_queue.pop_front();
        }
        Dispatch(record);
    }
}

void LoggerService::Dispatch(const LogRecord& record)
{
    std::scoped_lock lock(m_sinkMutex);
    for (auto& sink : m_sinks) sink->Write(record);
}
}
