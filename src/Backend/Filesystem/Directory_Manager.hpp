#pragma once

#include "Backend/Error/Result.hpp"

#include <filesystem>
#include <vector>

namespace Devilz::Backend
{
class Directory_Manager final
{
public:
    [[nodiscard]] Result<bool> Exists(const std::filesystem::path& path) const;
    Result<void> Create(const std::filesystem::path& path) const;
    Result<void> Ensure(const std::filesystem::path& path) const;
    Result<void> Remove(const std::filesystem::path& path, bool recursive = false) const;
    Result<void> Copy(const std::filesystem::path& from, const std::filesystem::path& to, bool recursive = true, bool overwrite = false) const;
    Result<void> Move(const std::filesystem::path& from, const std::filesystem::path& to) const;

    [[nodiscard]] Result<std::vector<std::filesystem::path>> Enumerate(
        const std::filesystem::path& path, bool recursive = false) const;
};
}
