#pragma once

#include "Backend/Error/Result.hpp"
#include "Backend/Filesystem/File_System_Manager.hpp"
#include "Module_Manager.hpp"
#include "Pattern_Cache.hpp"

#include <filesystem>

namespace Devilz::Backend
{
class Pattern_Cache_Persistence final
{
public:
    explicit Pattern_Cache_Persistence(File_System_Manager* files = nullptr) noexcept
        : m_files(files) {}

    void SetFileSystem(File_System_Manager* files) noexcept { m_files = files; }

    Result<void> Save(const Pattern_Cache& cache, const std::filesystem::path& path) const;
    Result<std::size_t> LoadForModule(Pattern_Cache& cache,
                                      const Module_Info& module,
                                      const std::filesystem::path& path) const;

private:
    File_System_Manager* m_files = nullptr;
};
}
