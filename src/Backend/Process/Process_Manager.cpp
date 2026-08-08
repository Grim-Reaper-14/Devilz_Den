#include "Process_Manager.hpp"

#include <Windows.h>
#include <TlHelp32.h>

#include <algorithm>
#include <cctype>

namespace Devilz::Backend
{
namespace
{
std::string WideToUtf8(const wchar_t* value)
{
    if (!value || *value == L'\0') return {};

    const int required = ::WideCharToMultiByte(
        CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
    if (required <= 1) return {};

    std::string result(static_cast<std::size_t>(required), '\0');
    const int written = ::WideCharToMultiByte(
        CP_UTF8, 0, value, -1, result.data(), required, nullptr, nullptr);
    if (written <= 1) return {};

    result.resize(static_cast<std::size_t>(written - 1));
    return result;
}

bool EqualInsensitive(std::string_view left, std::string_view right)
{
    if (left.size() != right.size()) return false;
    return std::equal(left.begin(), left.end(), right.begin(), right.end(), [](char a, char b) {
        return std::tolower(static_cast<unsigned char>(a)) ==
               std::tolower(static_cast<unsigned char>(b));
    });
}

Process_Architecture DetectArchitecture(HANDLE process) noexcept
{
    USHORT processMachine = IMAGE_FILE_MACHINE_UNKNOWN;
    USHORT nativeMachine = IMAGE_FILE_MACHINE_UNKNOWN;
    if (::IsWow64Process2(process, &processMachine, &nativeMachine)) {
        const USHORT machine = processMachine == IMAGE_FILE_MACHINE_UNKNOWN ? nativeMachine : processMachine;
        if (machine == IMAGE_FILE_MACHINE_AMD64) return Process_Architecture::X64;
        if (machine == IMAGE_FILE_MACHINE_I386) return Process_Architecture::X86;
        if (machine == IMAGE_FILE_MACHINE_ARM64) return Process_Architecture::Arm64;
    }
    return Process_Architecture::Unknown;
}
}

Result<Process_Info> Process_Manager::Inspect(std::uint32_t pid, std::string executableName) const
{
    HANDLE process = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process)
        return Result<Process_Info>::Failure(Error::FromWin32(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
            ::GetLastError(), "Unable to open process for inspection").With("PID", std::to_string(pid)));

    wchar_t pathBuffer[32768]{};
    DWORD pathLength = static_cast<DWORD>(std::size(pathBuffer));
    std::filesystem::path path;
    if (::QueryFullProcessImageNameW(process, 0, pathBuffer, &pathLength))
        path = std::filesystem::path(pathBuffer);

    Process_Info info{};
    info.pid = pid;
    info.executableName = std::move(executableName);
    info.executablePath = std::move(path);
    info.architecture = DetectArchitecture(process);
    info.running = true;
    ::CloseHandle(process);
    return Result<Process_Info>::Success(std::move(info));
}

Result<std::vector<Process_Info>> Process_Manager::Enumerate() const
{
    HANDLE snapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return Result<std::vector<Process_Info>>::Failure(Error::FromWin32(ErrorCode::RuntimeFailure,
            ErrorCategory::Runtime, ::GetLastError(), "Unable to create process snapshot"));

    std::vector<Process_Info> processes;
    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    if (::Process32FirstW(snapshot, &entry)) {
        do {
            auto inspected = Inspect(entry.th32ProcessID, WideToUtf8(entry.szExeFile));
            if (inspected) processes.push_back(std::move(inspected.Value()));
        } while (::Process32NextW(snapshot, &entry));
    }
    ::CloseHandle(snapshot);
    return Result<std::vector<Process_Info>>::Success(std::move(processes));
}

Result<Process_Info> Process_Manager::Find(std::string_view executableName) const
{
    auto processes = Enumerate();
    if (!processes) return Result<Process_Info>::Failure(processes.Failure());
    for (auto& process : processes.Value()) {
        if (EqualInsensitive(process.executableName, executableName))
            return Result<Process_Info>::Success(std::move(process));
    }
    return Result<Process_Info>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
        "Requested process is not running").With("Process", std::string(executableName)));
}

Result<bool> Process_Manager::IsRunning(std::string_view executableName) const
{
    auto found = Find(executableName);
    return Result<bool>::Success(static_cast<bool>(found));
}
}
