#include "Error.hpp"

#include <Windows.h>

#include <iomanip>
#include <sstream>

namespace Devilz::Backend
{
namespace
{
std::string FormatSystemMessage(std::uint32_t code)
{
    wchar_t* buffer = nullptr;
    const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    const DWORD length = ::FormatMessageW(flags, nullptr, code, 0,
        reinterpret_cast<wchar_t*>(&buffer), 0, nullptr);

    if (length == 0 || !buffer)
        return "No system message available.";

    const int utf8Length = ::WideCharToMultiByte(CP_UTF8, 0, buffer, static_cast<int>(length), nullptr, 0, nullptr, nullptr);
    std::string result(static_cast<std::size_t>(utf8Length), '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, buffer, static_cast<int>(length), result.data(), utf8Length, nullptr, nullptr);
    ::LocalFree(buffer);

    while (!result.empty() && (result.back() == '\r' || result.back() == '\n' || result.back() == ' '))
        result.pop_back();
    return result;
}
}

Error::Error(ErrorCode code, ErrorCategory category, std::string message, std::source_location source)
    : m_code(code), m_category(category), m_message(std::move(message)), m_source(source)
{
}

Error Error::FromWin32(ErrorCode code, ErrorCategory category, std::uint32_t nativeCode,
                       std::string message, std::source_location source)
{
    Error error(code, category, std::move(message), source);
    error.m_nativeCode = nativeCode;
    error.m_hasNativeCode = true;
    error.m_nativeMessage = FormatSystemMessage(nativeCode);
    return error;
}

Error Error::FromHRESULT(ErrorCode code, ErrorCategory category, long hresult,
                         std::string message, std::source_location source)
{
    Error error(code, category, std::move(message), source);
    error.m_hresult = hresult;
    error.m_hasHRESULT = true;
    error.m_nativeMessage = FormatSystemMessage(static_cast<std::uint32_t>(hresult));
    return error;
}

Error& Error::With(std::string key, std::string value)
{
    m_context.push_back({std::move(key), std::move(value)});
    return *this;
}

std::string Error::DetailedDescription() const
{
    std::ostringstream out;
    out << "ErrorCode=" << static_cast<std::uint32_t>(m_code)
        << " Category=" << static_cast<unsigned>(m_category)
        << " Message=\"" << m_message << "\"";

    if (m_hasNativeCode)
        out << " Win32=" << m_nativeCode << " (0x" << std::hex << std::uppercase << m_nativeCode << std::dec << ')';
    if (m_hasHRESULT)
        out << " HRESULT=0x" << std::hex << std::uppercase << static_cast<std::uint32_t>(m_hresult) << std::dec;
    if (!m_nativeMessage.empty())
        out << " NativeMessage=\"" << m_nativeMessage << "\"";

    out << " Source=" << m_source.file_name() << ':' << m_source.line()
        << " Function=\"" << m_source.function_name() << "\"";

    for (const auto& item : m_context)
        out << ' ' << item.key << "=\"" << item.value << "\"";
    return out.str();
}
}
