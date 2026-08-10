#include "ConsoleSink.hpp"

#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>

namespace Devilz::Backend
{
namespace
{
bool IsUsableConsoleHandle(HANDLE handle) noexcept
{
    if (handle == nullptr || handle == INVALID_HANDLE_VALUE)
        return false;

    DWORD mode = 0;
    return ::GetConsoleMode(handle, &mode) != FALSE;
}
}

ConsoleSink::ConsoleSink()
{
    m_output = ::GetStdHandle(STD_OUTPUT_HANDLE);
    if (!IsUsableConsoleHandle(m_output)) {
        if (::AllocConsole() != FALSE) {
            m_ownedConsole = true;
            ::SetConsoleTitleW(L"Devilz_Den Runtime");

            if (const auto window = ::GetConsoleWindow()) {
                if (const auto menu = ::GetSystemMenu(window, FALSE)) {
                    ::DeleteMenu(menu, SC_CLOSE, MF_BYCOMMAND);
                    ::DrawMenuBar(window);
                }
            }

            m_output = ::GetStdHandle(STD_OUTPUT_HANDLE);
        }
    }

    if (!IsUsableConsoleHandle(m_output)) {
        m_output = INVALID_HANDLE_VALUE;
        return;
    }

    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (::GetConsoleScreenBufferInfo(m_output, &info) != FALSE)
        m_defaultAttributes = info.wAttributes;
}

ConsoleSink::~ConsoleSink()
{
    std::scoped_lock lock(m_mutex);
    if (m_output != INVALID_HANDLE_VALUE)
        ::SetConsoleTextAttribute(m_output, m_defaultAttributes);

    if (m_ownedConsole)
        ::FreeConsole();
}

WORD ConsoleSink::ColorFor(LogLevel level) const noexcept
{
    switch (level) {
    case LogLevel::Trace:
    case LogLevel::Debug:
        return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    case LogLevel::Info:
        return FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    case LogLevel::Notice:
        return FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    case LogLevel::Warning:
        return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    case LogLevel::Error:
        return FOREGROUND_RED | FOREGROUND_INTENSITY;
    case LogLevel::Critical:
    case LogLevel::Fatal:
        return BACKGROUND_RED | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    default:
        return m_defaultAttributes;
    }
}

void ConsoleSink::Write(const LogRecord& record)
{
    std::scoped_lock lock(m_mutex);
    if (m_output == INVALID_HANDLE_VALUE)
        return;

    const auto time = std::chrono::system_clock::to_time_t(record.timestamp);
    std::tm local{};
    localtime_s(&local, &time);

    std::ostringstream out;
    out << std::put_time(&local, "%Y-%m-%d %H:%M:%S")
        << " [" << record.sequence << "] [" << record.service << "] " << record.message;
    if (record.error)
        out << " | " << record.error->DetailedDescription();
    out << '\n';

    const auto text = out.str();
    ::SetConsoleTextAttribute(m_output, ColorFor(record.level));

    DWORD written = 0;
    ::WriteConsoleA(m_output,
                    text.data(),
                    static_cast<DWORD>(text.size()),
                    &written,
                    nullptr);

    ::SetConsoleTextAttribute(m_output, m_defaultAttributes);
}
}
