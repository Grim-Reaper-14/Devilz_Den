#include "Folder_System_Manager.hpp"

#include <utility>

namespace Devilz::Backend
{
Folder_System_Manager::Folder_System_Manager(std::filesystem::path root)
    : m_root(std::move(root))
{
}

void Folder_System_Manager::SetRoot(std::filesystem::path root)
{
    m_root = std::move(root);
}

Result<void> Folder_System_Manager::RegisterFolder(std::string name, std::filesystem::path relativePath, bool create)
{
    if (name.empty())
        return Result<void>::Failure(Error(ErrorCode::DirectoryCreateFailed, ErrorCategory::Filesystem, "Folder registration requires a name"));

    const std::string key = name;
    m_folders.insert_or_assign(std::move(name), std::move(relativePath));
    if (!create)
        return Result<void>::Success();

    const auto it = m_folders.find(key);
    if (it == m_folders.end())
        return Result<void>::Failure(Error(ErrorCode::DirectoryCreateFailed, ErrorCategory::Filesystem,
            "Registered folder could not be resolved after insertion").With("Folder", key));

    return m_directories.Ensure(ResolveRelative(it->second));
}

Result<std::filesystem::path> Folder_System_Manager::Resolve(std::string_view name) const
{
    const auto it = m_folders.find(std::string(name));
    if (it == m_folders.end())
        return Result<std::filesystem::path>::Failure(
            Error(ErrorCode::DirectoryOpenFailed, ErrorCategory::Filesystem, "Folder is not registered")
                .With("Folder", std::string(name)));

    return Result<std::filesystem::path>::Success(ResolveRelative(it->second));
}

std::filesystem::path Folder_System_Manager::ResolveRelative(const std::filesystem::path& relativePath) const
{
    if (relativePath.is_absolute())
        return relativePath.lexically_normal();
    return (m_root / relativePath).lexically_normal();
}

Result<void> Folder_System_Manager::EnsureRegisteredFolders() const
{
    for (const auto& [name, relative] : m_folders) {
        auto result = m_directories.Ensure(ResolveRelative(relative));
        if (!result)
            return Result<void>::Failure(Error(result.Failure()).With("RegisteredFolder", name));
    }
    return Result<void>::Success();
}
}
