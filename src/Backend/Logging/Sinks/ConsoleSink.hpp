#pragma once

#include "Backend/Logging/ILogSink.hpp"

#include <Windows.h>

#include <mutex>

namespace Devilz::Backend
{
class ConsoleSink final : public ILogSink
{
public:
    ConsoleSink();
    ~ConsoleSink() override;

    ConsoleSink(const ConsoleSink&) = delete;
    ConsoleSink& operator=(const ConsoleSink&) = delete;

    void Write(const LogRecord& record) override;
    void Flush() override {}

private:
    [[nodiscard]] WORD ColorFor(LogLevel level) const noexcept;

    std::mutex m_mutex;
    HANDLE m_output = INVALID_HANDLE_VALUE;
    WORD m_defaultAttributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    bool m_ownedConsole = false;
};
}
