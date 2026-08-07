#pragma once

#include "Directory_Manager.hpp"

#include <filesystem>
#include <string>
#include <unordered_map>

namespace Devilz::Backend
{
class Folder_System_Manager final
{
public:
    explicit Folder_System_Manager(std::filesystem::path root = {});

    void SetRoot(std::filesystem::path root);
    [[nodiscard]] const std::filesystem::path& Root() const noexcept { return m_root; }

    Result<void> RegisterFolder(std::string name, std::filesystem::path relativePath, bool create = true);
    [[nodiscard]] Result<std::filesystem::path> Resolve(std::string_view name) const;
    [[nodiscard]] std::filesystem::path ResolveRelative(const std::filesystem::path& relativePath) const;
    Result<void> EnsureRegisteredFolders() const;

private:
    std::filesystem::path m_root;
    std::unordered_map<std::string, std::filesystem::path> m_folders;
    Directory_Manager m_directories;
};
}
