#include "ConsoleSink.hpp"

#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

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

std::string ServiceName(std::string_view service)
{
    if (service == "Runtime") return "RUNTIME";
    if (service == "Threading") return "THREADING";
    if (service == "GTA5_Enhanced") return "GTA";
    if (service == "GTA5_Enhanced.Targets") return "TARGET";
    if (service == "GTA5_Enhanced.Evidence") return "EVIDENCE";
    if (service == "GTA5_Enhanced.Natives") return "NATIVES";
    if (service == "GTA5_Enhanced.Frontend") return "FRONTEND";
    return std::string(service);
}

std::string SectionName(std::string_view service)
{
    if (service == "GTA5_Enhanced") return "GTA RUNTIME";
    if (service == "GTA5_Enhanced.Targets" || service == "GTA5_Enhanced.Evidence") return "TARGETS";
    if (service == "GTA5_Enhanced.Natives") return "NATIVES";
    if (service == "GTA5_Enhanced.Frontend") return "FRONTEND";
    return {};
}

std::vector<std::string_view> SplitFields(std::string_view text)
{
    std::vector<std::string_view> fields;
    std::size_t start = 0;
    while (start <= text.size()) {
        const auto separator = text.find(" | ", start);
        if (separator == std::string_view::npos) {
            fields.push_back(text.substr(start));
            break;
        }
        fields.push_back(text.substr(start, separator - start));
        start = separator + 3;
    }
    return fields;
}

bool IsNoisyEvidenceField(std::string_view field) noexcept
{
    return field.starts_with("PointeeQwords=") ||
           field.starts_with("Objects=") ||
           field.starts_with("DominantBytes=") ||
           field.starts_with("DominantQwords=") ||
           field.starts_with("Qwords=");
}

std::string FriendlyField(std::string_view field)
{
    const auto equals = field.find('=');
    if (equals == std::string_view::npos)
        return std::string(field);

    auto key = field.substr(0, equals);
    const auto value = field.substr(equals + 1);

    if (key == "Kind") key = "Kind";
    else if (key == "CandidateCommitted") key = "Committed";
    else if (key == "Readable") key = "Readable";
    else if (key == "Writable") key = "Writable";
    else if (key == "Executable") key = "Executable";
    else if (key == "Pointee") key = "Pointee";
    else if (key == "PointeeReadable") key = "Pointee Readable";
    else if (key == "PointeeExecutable") key = "Pointee Executable";
    else if (key == "ObjectSlots") key = "Object Slots";
    else if (key == "ReadableObjects") key = "Readable Objects";
    else if (key == "FirstQwordsDecoded") key = "First Qwords";
    else if (key == "FirstQwordExecImagePtrs") key = "Executable First Qwords";
    else if (key == "DominantFirstQword") key = "Dispatch Table";
    else if (key == "DominantCount") key = "Dominant Objects";
    else if (key == "DominantReadablePtrs") key = "Readable Dispatch Entries";
    else if (key == "DominantExecImagePtrs") key = "Executable Dispatch Entries";

    std::ostringstream out;
    out << std::left << std::setw(28) << key << value;
    return out.str();
}

void WriteText(HANDLE output, std::string_view text)
{
    DWORD written = 0;
    ::WriteConsoleA(output,
                    text.data(),
                    static_cast<DWORD>(text.size()),
                    &written,
                    nullptr);
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

    if (!m_bannerWritten) {
        ::SetConsoleTextAttribute(m_output, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        WriteText(m_output,
                  "============================================================\n"
                  " DEVILZ_DEN  |  GTA V ENHANCED RUNTIME\n"
                  "============================================================\n\n");
        m_bannerWritten = true;
    }

    const auto section = SectionName(record.service);
    if (!section.empty() && section != m_lastSection) {
        m_lastSection = section;
        ::SetConsoleTextAttribute(m_output, m_defaultAttributes);
        std::ostringstream separator;
        separator << '\n' << "-------------------- " << section << ' ';
        const auto used = 22U + section.size();
        if (used < 60U)
            separator << std::string(60U - used, '-');
        separator << "\n\n";
        WriteText(m_output, separator.str());
    }

    const auto time = std::chrono::system_clock::to_time_t(record.timestamp);
    std::tm local{};
    localtime_s(&local, &time);

    std::ostringstream prefix;
    prefix << std::put_time(&local, "%H:%M:%S") << "  "
           << std::left << std::setw(6) << LevelName(record.level) << ' '
           << std::left << std::setw(10) << ServiceName(record.service) << ' ';

    ::SetConsoleTextAttribute(m_output, ColorFor(record.level));
    WriteText(m_output, prefix.str());

    if (record.service == "GTA5_Enhanced.Evidence") {
        const auto colon = record.message.find(": ");
        if (colon != std::string::npos) {
            WriteText(m_output, record.message.substr(0, colon));
            WriteText(m_output, "\n");
            const auto fields = SplitFields(std::string_view(record.message).substr(colon + 2));
            std::size_t suppressed = 0;
            for (const auto field : fields) {
                if (IsNoisyEvidenceField(field)) {
                    ++suppressed;
                    continue;
                }
                WriteText(m_output, "                    ");
                WriteText(m_output, FriendlyField(field));
                WriteText(m_output, "\n");
            }
            if (suppressed != 0) {
                WriteText(m_output, "                    Full raw evidence          see Devilz_Den.log\n");
            }
        } else {
            WriteText(m_output, record.message);
            WriteText(m_output, "\n");
        }
    } else {
        const auto fields = SplitFields(record.message);
        if (fields.size() <= 1) {
            WriteText(m_output, record.message);
            WriteText(m_output, "\n");
        } else {
            WriteText(m_output, fields.front());
            WriteText(m_output, "\n");
            for (std::size_t index = 1; index < fields.size(); ++index) {
                WriteText(m_output, "                    ");
                WriteText(m_output, FriendlyField(fields[index]));
                WriteText(m_output, "\n");
            }
        }
    }

    if (record.error) {
        WriteText(m_output, "                    Error Details              ");
        WriteText(m_output, record.error->DetailedDescription());
        WriteText(m_output, "\n");
    }

    ::SetConsoleTextAttribute(m_output, m_defaultAttributes);
}
}
