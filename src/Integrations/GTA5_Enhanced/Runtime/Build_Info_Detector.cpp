#include "Build_Info_Detector.hpp"

#include <Windows.h>

#include <fstream>
#include <sstream>

namespace Devilz::Integrations::GTA5_Enhanced
{
using namespace Devilz::Backend;

namespace
{
Build_Info MakeBuildInfo(const Process_Info& process, std::uint32_t timestamp, std::uint32_t imageSize)
{
    Build_Info build{};
    build.peTimestamp = timestamp;
    build.imageSize = imageSize;
    build.executablePath = process.executablePath;
    build.fingerprint = (static_cast<std::uint64_t>(build.peTimestamp) << 32) |
                        static_cast<std::uint64_t>(build.imageSize);

    std::ostringstream version;
    version << "PE-" << build.peTimestamp << '-' << build.imageSize;
    build.version = version.str();
    return build;
}

Result<Build_Info> DetectLoadedImage(const Process_Info& process)
{
    HMODULE module = ::GetModuleHandleW(nullptr);
    if (!module)
        return Result<Build_Info>::Failure(Error::FromWin32(
            ErrorCode::RuntimeFailure,
            ErrorCategory::Runtime,
            ::GetLastError(),
            "Unable to resolve current process image module"));

    const auto base = reinterpret_cast<std::uintptr_t>(module);
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0)
        return Result<Build_Info>::Failure(Error(
            ErrorCode::RuntimeFailure,
            ErrorCategory::Runtime,
            "Loaded GTA image has an invalid DOS header"));

    const auto ntAddress = base + static_cast<std::uintptr_t>(dos->e_lfanew);
    const auto* signature = reinterpret_cast<const DWORD*>(ntAddress);
    if (*signature != IMAGE_NT_SIGNATURE)
        return Result<Build_Info>::Failure(Error(
            ErrorCode::RuntimeFailure,
            ErrorCategory::Runtime,
            "Loaded GTA image has an invalid PE signature"));

    const auto* fileHeader = reinterpret_cast<const IMAGE_FILE_HEADER*>(ntAddress + sizeof(DWORD));
    const auto optionalAddress = ntAddress + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER);
    const auto magic = *reinterpret_cast<const WORD*>(optionalAddress);

    std::uint32_t imageSize = 0;
    if (magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        const auto* optional = reinterpret_cast<const IMAGE_OPTIONAL_HEADER64*>(optionalAddress);
        imageSize = optional->SizeOfImage;
    } else if (magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
        const auto* optional = reinterpret_cast<const IMAGE_OPTIONAL_HEADER32*>(optionalAddress);
        imageSize = optional->SizeOfImage;
    } else {
        return Result<Build_Info>::Failure(Error(
            ErrorCode::RuntimeFailure,
            ErrorCategory::Runtime,
            "Loaded GTA image uses an unsupported PE optional header"));
    }

    if (imageSize == 0)
        return Result<Build_Info>::Failure(Error(
            ErrorCode::RuntimeFailure,
            ErrorCategory::Runtime,
            "Loaded GTA image reported an invalid image size"));

    return Result<Build_Info>::Success(MakeBuildInfo(process, fileHeader->TimeDateStamp, imageSize));
}
}

Result<Build_Info> Build_Info_Detector::Detect(const Process_Info& process) const
{
    if (!process.Valid() || process.executablePath.empty())
        return Result<Build_Info>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
            "Cannot detect GTA build from invalid process information"));

    if (process.pid == ::GetCurrentProcessId())
        return DetectLoadedImage(process);

    std::ifstream file(process.executablePath, std::ios::binary);
    if (!file)
        return Result<Build_Info>::Failure(Error(ErrorCode::FileOpenFailed, ErrorCategory::Filesystem,
            "Unable to open GTA executable for build inspection")
            .With("Path", process.executablePath.string()));

    IMAGE_DOS_HEADER dos{};
    file.read(reinterpret_cast<char*>(&dos), sizeof(dos));
    if (!file || dos.e_magic != IMAGE_DOS_SIGNATURE || dos.e_lfanew <= 0)
        return Result<Build_Info>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
            "GTA executable has an invalid DOS/PE header"));

    file.seekg(dos.e_lfanew, std::ios::beg);
    DWORD signature = 0;
    IMAGE_FILE_HEADER fileHeader{};
    file.read(reinterpret_cast<char*>(&signature), sizeof(signature));
    file.read(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));
    if (!file || signature != IMAGE_NT_SIGNATURE)
        return Result<Build_Info>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
            "GTA executable does not contain a valid PE signature"));

    WORD magic = 0;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    if (!file)
        return Result<Build_Info>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
            "Unable to read GTA optional-header magic"));

    file.seekg(-static_cast<std::streamoff>(sizeof(magic)), std::ios::cur);
    std::uint32_t imageSize = 0;
    if (magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        IMAGE_OPTIONAL_HEADER64 optional{};
        file.read(reinterpret_cast<char*>(&optional), sizeof(optional));
        imageSize = optional.SizeOfImage;
    } else if (magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
        IMAGE_OPTIONAL_HEADER32 optional{};
        file.read(reinterpret_cast<char*>(&optional), sizeof(optional));
        imageSize = optional.SizeOfImage;
    } else {
        return Result<Build_Info>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
            "GTA executable uses an unsupported PE optional header"));
    }

    if (!file || imageSize == 0)
        return Result<Build_Info>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
            "Unable to determine GTA image size"));

    return Result<Build_Info>::Success(MakeBuildInfo(process, fileHeader.TimeDateStamp, imageSize));
}
}
