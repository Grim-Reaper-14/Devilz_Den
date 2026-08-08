#include "Process_Module_Manager.hpp"

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
    const int required = ::WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
    if (required <= 1) return {};
    std::string result(static_cast<std::size_t>(required), '\0');
    const int written = ::WideCharToMultiByte(CP_UTF8, 0, value, -1, result.data(), required, nullptr, nullptr);
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
}

Result<std::vector<Process_Module_Info>> Process_Module_Manager::Enumerate() const
{
    if (m_pid == 0)
        return Result<std::vector<Process_Module_Info>>::Failure(
            Error(ErrorCode::InvalidArgument, ErrorCategory::Runtime, "Process module enumeration requires a valid PID"));

    HANDLE snapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, m_pid);
    if (snapshot == INVALID_HANDLE_VALUE)
        return Result<std::vector<Process_Module_Info>>::Failure(
            Error::FromWin32(ErrorCode::RuntimeFailure, ErrorCategory::Runtime, ::GetLastError(),
                "Unable to create target process module snapshot").With("PID", std::to_string(m_pid)));

    std::vector<Process_Module_Info> modules;
    MODULEENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    if (::Module32FirstW(snapshot, &entry)) {
        do {
            Process_Module_Info info{};
            info.name = WideToUtf8(entry.szModule);
            info.path = std::filesystem::path(entry.szExePath);
            info.baseAddress = reinterpret_cast<std::uintptr_t>(entry.modBaseAddr);
            info.imageSize = entry.modBaseSize;
            if (info.Valid()) modules.push_back(std::move(info));
        } while (::Module32NextW(snapshot, &entry));
    } else {
        const DWORD error = ::GetLastError();
        ::CloseHandle(snapshot);
        return Result<std::vector<Process_Module_Info>>::Failure(
            Error::FromWin32(ErrorCode::RuntimeFailure, ErrorCategory::Runtime, error,
                "Unable to enumerate target process modules").With("PID", std::to_string(m_pid)));
    }

    ::CloseHandle(snapshot);
    return Result<std::vector<Process_Module_Info>>::Success(std::move(modules));
}

Result<Process_Module_Info> Process_Module_Manager::Find(std::string_view moduleName) const
{
    auto modules = Enumerate();
    if (!modules) return Result<Process_Module_Info>::Failure(modules.Failure());

    for (auto& module : modules.Value()) {
        if (EqualInsensitive(module.name, moduleName))
            return Result<Process_Module_Info>::Success(std::move(module));
    }

    return Result<Process_Module_Info>::Failure(
        Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime, "Requested module is not loaded in target process")
            .With("PID", std::to_string(m_pid))
            .With("Module", std::string(moduleName)));
}
}
