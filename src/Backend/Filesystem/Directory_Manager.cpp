#include "Directory_Manager.hpp"

namespace Devilz::Backend
{
namespace
{
Error DirectoryError(ErrorCode code, std::string message, const std::filesystem::path& path, const std::error_code& ec)
{
    return Error::FromWin32(code, ErrorCategory::Filesystem,
                            static_cast<std::uint32_t>(ec.value()), std::move(message))
        .With("Path", path.string())
        .With("SystemCategory", ec.category().name())
        .With("SystemMessage", ec.message());
}
}

Result<bool> Directory_Manager::Exists(const std::filesystem::path& path) const
{
    std::error_code ec;
    const bool result = std::filesystem::is_directory(path, ec);
    if (ec)
        return Result<bool>::Failure(DirectoryError(ErrorCode::DirectoryOpenFailed, "Unable to query directory", path, ec));
    return Result<bool>::Success(result);
}

Result<void> Directory_Manager::Create(const std::filesystem::path& path) const
{
    std::error_code ec;
    std::filesystem::create_directories(path, ec);
    if (ec)
        return Result<void>::Failure(DirectoryError(ErrorCode::DirectoryCreateFailed, "Unable to create directory", path, ec));
    return Result<void>::Success();
}

Result<void> Directory_Manager::Ensure(const std::filesystem::path& path) const
{
    auto exists = Exists(path);
    if (!exists) return Result<void>::Failure(exists.Failure());
    if (exists.Value()) return Result<void>::Success();
    return Create(path);
}

Result<void> Directory_Manager::Remove(const std::filesystem::path& path, bool recursive) const
{
    std::error_code ec;
    if (recursive) std::filesystem::remove_all(path, ec);
    else std::filesystem::remove(path, ec);
    if (ec)
        return Result<void>::Failure(DirectoryError(ErrorCode::DirectoryOpenFailed, "Unable to remove directory", path, ec));
    return Result<void>::Success();
}

Result<void> Directory_Manager::Copy(const std::filesystem::path& from, const std::filesystem::path& to, bool recursive, bool overwrite) const
{
    std::error_code ec;
    auto options = std::filesystem::copy_options::none;
    if (recursive) options |= std::filesystem::copy_options::recursive;
    if (overwrite) options |= std::filesystem::copy_options::overwrite_existing;
    std::filesystem::copy(from, to, options, ec);
    if (ec)
        return Result<void>::Failure(DirectoryError(ErrorCode::DirectoryOpenFailed, "Unable to copy directory", from, ec).With("Destination", to.string()));
    return Result<void>::Success();
}

Result<void> Directory_Manager::Move(const std::filesystem::path& from, const std::filesystem::path& to) const
{
    std::error_code ec;
    std::filesystem::rename(from, to, ec);
    if (ec)
        return Result<void>::Failure(DirectoryError(ErrorCode::DirectoryOpenFailed, "Unable to move directory", from, ec).With("Destination", to.string()));
    return Result<void>::Success();
}

Result<std::vector<std::filesystem::path>> Directory_Manager::Enumerate(const std::filesystem::path& path, bool recursive) const
{
    std::vector<std::filesystem::path> result;
    std::error_code ec;
    if (recursive) {
        std::filesystem::recursive_directory_iterator it(path, std::filesystem::directory_options::skip_permission_denied, ec), end;
        if (ec) return Result<std::vector<std::filesystem::path>>::Failure(DirectoryError(ErrorCode::DirectoryOpenFailed, "Unable to enumerate directory", path, ec));
        for (; it != end; it.increment(ec)) {
            if (ec) return Result<std::vector<std::filesystem::path>>::Failure(DirectoryError(ErrorCode::DirectoryOpenFailed, "Directory enumeration failed", path, ec));
            result.push_back(it->path());
        }
    } else {
        std::filesystem::directory_iterator it(path, std::filesystem::directory_options::skip_permission_denied, ec), end;
        if (ec) return Result<std::vector<std::filesystem::path>>::Failure(DirectoryError(ErrorCode::DirectoryOpenFailed, "Unable to enumerate directory", path, ec));
        for (; it != end; it.increment(ec)) {
            if (ec) return Result<std::vector<std::filesystem::path>>::Failure(DirectoryError(ErrorCode::DirectoryOpenFailed, "Directory enumeration failed", path, ec));
            result.push_back(it->path());
        }
    }
    return Result<std::vector<std::filesystem::path>>::Success(std::move(result));
}
}
