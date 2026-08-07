#pragma once

#include "Backend/Error/Error.hpp"

#include <chrono>
#include <cstdint>
#include <optional>
#include <source_location>
#include <string>
#include <thread>

namespace Devilz::Backend
{
enum class LogLevel : std::uint8_t { Trace, Debug, Info, Notice, Warning, Error, Critical, Fatal };

struct LogRecord
{
    std::uint64_t sequence = 0;
    LogLevel level = LogLevel::Info;
    std::chrono::system_clock::time_point timestamp{};
    std::string message;
    std::string service;
    std::string threadName;
    std::thread::id threadId{};
    std::uint64_t taskId = 0;
    std::uint64_t correlationId = 0;
    std::source_location source = std::source_location::current();
    std::optional<Error> error;
};
}
