#pragma once

#include "LoggerService.hpp"

#include <atomic>
#include <memory>
#include <source_location>
#include <string>
#include <string_view>
#include <utility>

namespace Devilz::Backend
{
class Logger_Channel;

// High-level logging facade for the Devilz_Den backend runtime.
//
// LoggerService remains responsible for asynchronous queue processing and sink
// dispatch. Logger_Manager owns that service and provides application-wide
// lifecycle, severity filtering, sink registration, and convenience helpers.
class Logger_Manager final
{
public:
    static Logger_Manager& Instance() noexcept
    {
        static Logger_Manager instance;
        return instance;
    }

    Logger_Manager(const Logger_Manager&) = delete;
    Logger_Manager& operator=(const Logger_Manager&) = delete;
    Logger_Manager(Logger_Manager&&) = delete;
    Logger_Manager& operator=(Logger_Manager&&) = delete;

    void Start()
    {
        if (m_running.exchange(true, std::memory_order_acq_rel))
            return;

        m_service.Start();
    }

    void Stop()
    {
        if (!m_running.exchange(false, std::memory_order_acq_rel))
            return;

        m_service.Stop();
    }

    void Flush()
    {
        m_service.Flush();
    }

    [[nodiscard]] bool IsRunning() const noexcept
    {
        return m_running.load(std::memory_order_acquire);
    }

    void SetMinimumLevel(LogLevel level) noexcept
    {
        m_minimumLevel.store(level, std::memory_order_release);
    }

    [[nodiscard]] LogLevel GetMinimumLevel() const noexcept
    {
        return m_minimumLevel.load(std::memory_order_acquire);
    }

    void AddSink(std::unique_ptr<ILogSink> sink)
    {
        if (sink)
            m_service.AddSink(std::move(sink));
    }

    void Log(LogLevel level,
             std::string message,
             std::string service = {},
             std::source_location source = std::source_location::current())
    {
        if (!ShouldLog(level))
            return;

        m_service.Log(level, std::move(message), std::move(service), source);
    }

    void LogError(LogLevel level,
                  const Error& error,
                  std::string service = {},
                  std::source_location source = std::source_location::current())
    {
        if (!ShouldLog(level))
            return;

        m_service.LogError(level, error, std::move(service), source);
    }

    void LogWithContext(LogLevel level,
                        std::string message,
                        LogContext context,
                        std::source_location source = std::source_location::current())
    {
        if (!ShouldLog(level))
            return;

        m_service.LogWithContext(level, std::move(message), std::move(context), source);
    }

    void LogErrorWithContext(LogLevel level,
                             const Error& error,
                             LogContext context,
                             std::source_location source = std::source_location::current())
    {
        if (!ShouldLog(level))
            return;

        m_service.LogErrorWithContext(level, error, std::move(context), source);
    }

    [[nodiscard]] Logger_Channel Channel(std::string service);

    void Trace(std::string message,
               std::string service = {},
               std::source_location source = std::source_location::current())
    {
        Log(LogLevel::Trace, std::move(message), std::move(service), source);
    }

    void Debug(std::string message,
               std::string service = {},
               std::source_location source = std::source_location::current())
    {
        Log(LogLevel::Debug, std::move(message), std::move(service), source);
    }

    void Info(std::string message,
              std::string service = {},
              std::source_location source = std::source_location::current())
    {
        Log(LogLevel::Info, std::move(message), std::move(service), source);
    }

    void Notice(std::string message,
                std::string service = {},
                std::source_location source = std::source_location::current())
    {
        Log(LogLevel::Notice, std::move(message), std::move(service), source);
    }

    void Warning(std::string message,
                 std::string service = {},
                 std::source_location source = std::source_location::current())
    {
        Log(LogLevel::Warning, std::move(message), std::move(service), source);
    }

    void ErrorMessage(std::string message,
                      std::string service = {},
                      std::source_location source = std::source_location::current())
    {
        Log(LogLevel::Error, std::move(message), std::move(service), source);
    }

    void Critical(std::string message,
                  std::string service = {},
                  std::source_location source = std::source_location::current())
    {
        Log(LogLevel::Critical, std::move(message), std::move(service), source);
    }

    void Fatal(std::string message,
               std::string service = {},
               std::source_location source = std::source_location::current())
    {
        Log(LogLevel::Fatal, std::move(message), std::move(service), source);
        Flush();
    }

    [[nodiscard]] LoggerService& Service() noexcept
    {
        return m_service;
    }

    [[nodiscard]] const LoggerService& Service() const noexcept
    {
        return m_service;
    }

private:
    Logger_Manager() = default;

    ~Logger_Manager()
    {
        Stop();
    }

    [[nodiscard]] bool ShouldLog(LogLevel level) const noexcept
    {
        return static_cast<unsigned>(level) >=
               static_cast<unsigned>(m_minimumLevel.load(std::memory_order_acquire));
    }

    LoggerService m_service;
    std::atomic<LogLevel> m_minimumLevel{LogLevel::Trace};
    std::atomic_bool m_running{false};
};

class Logger_Channel final
{
public:
    Logger_Channel(Logger_Manager& logger, LogContext context)
        : m_logger(&logger), m_context(std::move(context))
    {
    }

    [[nodiscard]] Logger_Channel Child(std::string_view child) const
    {
        auto context = m_context;
        if (!child.empty()) {
            if (!context.service.empty())
                context.service.push_back('.');
            context.service.append(child.data(), child.size());
        }
        return Logger_Channel(*m_logger, std::move(context));
    }

    [[nodiscard]] Logger_Channel WithThreadName(std::string threadName) const
    {
        auto context = m_context;
        context.threadName = std::move(threadName);
        return Logger_Channel(*m_logger, std::move(context));
    }

    [[nodiscard]] Logger_Channel WithTask(std::uint64_t taskId) const
    {
        auto context = m_context;
        context.taskId = taskId;
        return Logger_Channel(*m_logger, std::move(context));
    }

    [[nodiscard]] Logger_Channel WithCorrelation(std::uint64_t correlationId) const
    {
        auto context = m_context;
        context.correlationId = correlationId;
        return Logger_Channel(*m_logger, std::move(context));
    }

    [[nodiscard]] const LogContext& Context() const noexcept { return m_context; }

    void Log(LogLevel level,
             std::string message,
             std::source_location source = std::source_location::current()) const
    {
        m_logger->LogWithContext(level, std::move(message), m_context, source);
    }

    void LogError(LogLevel level,
                  const Error& error,
                  std::source_location source = std::source_location::current()) const
    {
        m_logger->LogErrorWithContext(level, error, m_context, source);
    }

    void Trace(std::string message, std::source_location source = std::source_location::current()) const
    { Log(LogLevel::Trace, std::move(message), source); }

    void Debug(std::string message, std::source_location source = std::source_location::current()) const
    { Log(LogLevel::Debug, std::move(message), source); }

    void Info(std::string message, std::source_location source = std::source_location::current()) const
    { Log(LogLevel::Info, std::move(message), source); }

    void Notice(std::string message, std::source_location source = std::source_location::current()) const
    { Log(LogLevel::Notice, std::move(message), source); }

    void Warning(std::string message, std::source_location source = std::source_location::current()) const
    { Log(LogLevel::Warning, std::move(message), source); }

    void ErrorMessage(std::string message, std::source_location source = std::source_location::current()) const
    { Log(LogLevel::Error, std::move(message), source); }

    void Critical(std::string message, std::source_location source = std::source_location::current()) const
    { Log(LogLevel::Critical, std::move(message), source); }

    void Fatal(std::string message, std::source_location source = std::source_location::current()) const
    {
        Log(LogLevel::Fatal, std::move(message), source);
        m_logger->Flush();
    }

private:
    Logger_Manager* m_logger = nullptr;
    LogContext m_context;
};

inline Logger_Channel Logger_Manager::Channel(std::string service)
{
    LogContext context;
    context.service = std::move(service);
    return Logger_Channel(*this, std::move(context));
}
}
