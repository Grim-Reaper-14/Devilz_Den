#include "Process_Memory_Reader.hpp"

#include <Windows.h>

namespace Devilz::Backend
{
Result<void> Process_Memory_Reader::ReadInto(std::uintptr_t address, std::span<std::byte> destination) const
{
    if (m_pid == 0 || address == 0 || destination.empty())
        return Result<void>::Failure(Error(ErrorCode::InvalidArgument, ErrorCategory::Runtime,
            "Invalid process memory read request"));

    HANDLE process = ::OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, m_pid);
    if (!process)
        return Result<void>::Failure(Error::FromWin32(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
            ::GetLastError(), "Unable to open target process for read-only memory access")
            .With("PID", std::to_string(m_pid)));

    SIZE_T bytesRead = 0;
    const BOOL ok = ::ReadProcessMemory(process,
        reinterpret_cast<LPCVOID>(address), destination.data(), destination.size(), &bytesRead);
    const DWORD error = ok ? ERROR_SUCCESS : ::GetLastError();
    ::CloseHandle(process);

    if (!ok || bytesRead != destination.size())
        return Result<void>::Failure(Error::FromWin32(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
            error, "Unable to read requested target process memory range")
            .With("PID", std::to_string(m_pid))
            .With("Address", std::to_string(address))
            .With("RequestedBytes", std::to_string(destination.size()))
            .With("ReadBytes", std::to_string(bytesRead)));

    return Result<void>::Success();
}

Result<std::vector<std::byte>> Process_Memory_Reader::Read(std::uintptr_t address, std::size_t size) const
{
    if (size == 0)
        return Result<std::vector<std::byte>>::Failure(Error(ErrorCode::InvalidArgument, ErrorCategory::Runtime,
            "Process memory read size cannot be zero"));

    std::vector<std::byte> buffer(size);
    auto result = ReadInto(address, buffer);
    if (!result)
        return Result<std::vector<std::byte>>::Failure(result.Failure());
    return Result<std::vector<std::byte>>::Success(std::move(buffer));
}
}
