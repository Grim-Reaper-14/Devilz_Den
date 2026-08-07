#pragma once

#include <cstdint>
#include <source_location>
#include <string>
#include <utility>
#include <vector>

namespace Devilz::Backend
{
enum class ErrorCategory : std::uint8_t
{
    None,
    Runtime,
    Service,
    Threading,
    Platform,
    Filesystem,
    Graphics,
    Network,
    Configuration,
    Scripting,
    Unknown
};

enum class ErrorCode : std::uint32_t
{
    None = 0,
    RuntimeFailure = 1000,
    ServiceInitializationFailed = 1100,
    ServiceDependencyMissing,
    ThreadCreationFailed = 2000,
    TaskExecutionFailed,
    ExecutorStopped,
    WindowFailure = 3000,
    DirectoryOpenFailed = 4000,
    DirectoryCreateFailed,
    FileOpenFailed,
    FileReadFailed,
    FileWriteFailed,
    D3D12Failure = 5000,
    D3D12DeviceCreationFailed,
    D3D12DeviceRemoved,
    NetworkFailure = 6000,
    Unknown = 0xFFFFFFFFu
};

struct ErrorContext
{
    std::string key;
    std::string value;
};

class Error
{
public:
    Error() = default;
    Error(ErrorCode code, ErrorCategory category, std::string message,
          std::source_location source = std::source_location::current());

    static Error FromWin32(ErrorCode code, ErrorCategory category, std::uint32_t nativeCode,
                           std::string message,
                           std::source_location source = std::source_location::current());
    static Error FromHRESULT(ErrorCode code, ErrorCategory category, long hresult,
                             std::string message,
                             std::source_location source = std::source_location::current());

    Error& With(std::string key, std::string value);

    [[nodiscard]] ErrorCode Code() const noexcept { return m_code; }
    [[nodiscard]] ErrorCategory Category() const noexcept { return m_category; }
    [[nodiscard]] const std::string& Message() const noexcept { return m_message; }
    [[nodiscard]] std::uint32_t NativeCode() const noexcept { return m_nativeCode; }
    [[nodiscard]] long HResult() const noexcept { return m_hresult; }
    [[nodiscard]] bool HasNativeCode() const noexcept { return m_hasNativeCode; }
    [[nodiscard]] bool HasHRESULT() const noexcept { return m_hasHRESULT; }
    [[nodiscard]] const std::string& NativeMessage() const noexcept { return m_nativeMessage; }
    [[nodiscard]] const std::source_location& Source() const noexcept { return m_source; }
    [[nodiscard]] const std::vector<ErrorContext>& Context() const noexcept { return m_context; }
    [[nodiscard]] std::string DetailedDescription() const;

private:
    ErrorCode m_code = ErrorCode::None;
    ErrorCategory m_category = ErrorCategory::None;
    std::string m_message;
    std::uint32_t m_nativeCode = 0;
    long m_hresult = 0;
    bool m_hasNativeCode = false;
    bool m_hasHRESULT = false;
    std::string m_nativeMessage;
    std::source_location m_source = std::source_location::current();
    std::vector<ErrorContext> m_context;
};
}
