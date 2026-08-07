#include "File_System_Manager.hpp"

#include <fstream>
#include <iterator>

namespace Devilz::Backend
{
namespace
{
Error FsError(ErrorCode code, std::string message, const std::filesystem::path& path, const std::error_code& ec)
{
    return Error::FromWin32(code, ErrorCategory::Filesystem,
                            static_cast<std::uint32_t>(ec.value()), std::move(message))
        .With("Path", path.string())
        .With("SystemCategory", ec.category().name())
        .With("SystemMessage", ec.message());
}

Result<void> EnsureParent(const std::filesystem::path& path)
{
    const auto parent = path.parent_path();
    if (parent.empty())
        return Result<void>::Success();

    std::error_code ec;
    std::filesystem::create_directories(parent, ec);
    if (ec)
        return Result<void>::Failure(FsError(ErrorCode::DirectoryCreateFailed, "Unable to create parent directory", parent, ec));

    return Result<void>::Success();
}
}

Result<bool> File_System_Manager::Exists(const std::filesystem::path& path) const
{
    std::error_code ec;
    const bool exists = std::filesystem::exists(path, ec);
    if (ec)
        return Result<bool>::Failure(FsError(ErrorCode::FileOpenFailed, "Unable to query file existence", path, ec));
    return Result<bool>::Success(exists);
}

Result<std::uintmax_t> File_System_Manager::Size(const std::filesystem::path& path) const
{
    std::error_code ec;
    const auto size = std::filesystem::file_size(path, ec);
    if (ec)
        return Result<std::uintmax_t>::Failure(FsError(ErrorCode::FileReadFailed, "Unable to query file size", path, ec));
    return Result<std::uintmax_t>::Success(size);
}

Result<std::string> File_System_Manager::ReadText(const std::filesystem::path& path) const
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        return Result<std::string>::Failure(Error(ErrorCode::FileOpenFailed, ErrorCategory::Filesystem, "Unable to open text file").With("Path", path.string()));

    std::string data((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    if (stream.bad())
        return Result<std::string>::Failure(Error(ErrorCode::FileReadFailed, ErrorCategory::Filesystem, "Unable to read text file").With("Path", path.string()));
    return Result<std::string>::Success(std::move(data));
}

Result<File_System_Manager::ByteBuffer> File_System_Manager::ReadBinary(const std::filesystem::path& path) const
{
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream)
        return Result<ByteBuffer>::Failure(Error(ErrorCode::FileOpenFailed, ErrorCategory::Filesystem, "Unable to open binary file").With("Path", path.string()));

    const auto end = stream.tellg();
    if (end < 0)
        return Result<ByteBuffer>::Failure(Error(ErrorCode::FileReadFailed, ErrorCategory::Filesystem, "Unable to determine binary file size").With("Path", path.string()));

    ByteBuffer data(static_cast<std::size_t>(end));
    stream.seekg(0, std::ios::beg);
    if (!data.empty())
        stream.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!stream)
        return Result<ByteBuffer>::Failure(Error(ErrorCode::FileReadFailed, ErrorCategory::Filesystem, "Unable to read binary file").With("Path", path.string()));
    return Result<ByteBuffer>::Success(std::move(data));
}

Result<void> File_System_Manager::WriteText(const std::filesystem::path& path, std::string_view text, bool createParents) const
{
    if (createParents) {
        auto result = EnsureParent(path);
        if (!result) return result;
    }
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream)
        return Result<void>::Failure(Error(ErrorCode::FileWriteFailed, ErrorCategory::Filesystem, "Unable to open text file for writing").With("Path", path.string()));
    stream.write(text.data(), static_cast<std::streamsize>(text.size()));
    if (!stream)
        return Result<void>::Failure(Error(ErrorCode::FileWriteFailed, ErrorCategory::Filesystem, "Unable to write text file").With("Path", path.string()));
    return Result<void>::Success();
}

Result<void> File_System_Manager::WriteBinary(const std::filesystem::path& path, std::span<const std::byte> data, bool createParents) const
{
    if (createParents) {
        auto result = EnsureParent(path);
        if (!result) return result;
    }
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream)
        return Result<void>::Failure(Error(ErrorCode::FileWriteFailed, ErrorCategory::Filesystem, "Unable to open binary file for writing").With("Path", path.string()));
    if (!data.empty())
        stream.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size_bytes()));
    if (!stream)
        return Result<void>::Failure(Error(ErrorCode::FileWriteFailed, ErrorCategory::Filesystem, "Unable to write binary file").With("Path", path.string()));
    return Result<void>::Success();
}

Result<void> File_System_Manager::AppendText(const std::filesystem::path& path, std::string_view text, bool createParents) const
{
    if (createParents) {
        auto result = EnsureParent(path);
        if (!result) return result;
    }
    std::ofstream stream(path, std::ios::binary | std::ios::app);
    if (!stream)
        return Result<void>::Failure(Error(ErrorCode::FileWriteFailed, ErrorCategory::Filesystem, "Unable to open text file for append").With("Path", path.string()));
    stream.write(text.data(), static_cast<std::streamsize>(text.size()));
    if (!stream)
        return Result<void>::Failure(Error(ErrorCode::FileWriteFailed, ErrorCategory::Filesystem, "Unable to append text file").With("Path", path.string()));
    return Result<void>::Success();
}

Result<void> File_System_Manager::Copy(const std::filesystem::path& from, const std::filesystem::path& to, bool overwrite) const
{
    auto parent = EnsureParent(to);
    if (!parent) return parent;
    std::error_code ec;
    const auto options = overwrite ? std::filesystem::copy_options::overwrite_existing : std::filesystem::copy_options::none;
    if (!std::filesystem::copy_file(from, to, options, ec) && ec)
        return Result<void>::Failure(FsError(ErrorCode::FileWriteFailed, "Unable to copy file", from, ec).With("Destination", to.string()));
    return Result<void>::Success();
}

Result<void> File_System_Manager::Move(const std::filesystem::path& from, const std::filesystem::path& to, bool overwrite) const
{
    auto parent = EnsureParent(to);
    if (!parent) return parent;
    std::error_code ec;
    if (overwrite && std::filesystem::exists(to, ec)) {
        ec.clear();
        std::filesystem::remove(to, ec);
        if (ec)
            return Result<void>::Failure(FsError(ErrorCode::FileWriteFailed, "Unable to replace destination file", to, ec));
    }
    ec.clear();
    std::filesystem::rename(from, to, ec);
    if (ec)
        return Result<void>::Failure(FsError(ErrorCode::FileWriteFailed, "Unable to move file", from, ec).With("Destination", to.string()));
    return Result<void>::Success();
}

Result<void> File_System_Manager::Remove(const std::filesystem::path& path) const
{
    std::error_code ec;
    std::filesystem::remove(path, ec);
    if (ec)
        return Result<void>::Failure(FsError(ErrorCode::FileWriteFailed, "Unable to remove file", path, ec));
    return Result<void>::Success();
}
}
