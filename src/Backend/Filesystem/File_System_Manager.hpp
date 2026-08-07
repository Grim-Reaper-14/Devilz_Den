#pragma once

#include "Backend/Error/Result.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace Devilz::Backend
{
class File_System_Manager final
{
public:
    using ByteBuffer = std::vector<std::byte>;

    [[nodiscard]] Result<bool> Exists(const std::filesystem::path& path) const;
    [[nodiscard]] Result<std::uintmax_t> Size(const std::filesystem::path& path) const;
    [[nodiscard]] Result<std::string> ReadText(const std::filesystem::path& path) const;
    [[nodiscard]] Result<ByteBuffer> ReadBinary(const std::filesystem::path& path) const;

    Result<void> WriteText(const std::filesystem::path& path, std::string_view text, bool createParents = true) const;
    Result<void> WriteBinary(const std::filesystem::path& path, std::span<const std::byte> data, bool createParents = true) const;
    Result<void> AppendText(const std::filesystem::path& path, std::string_view text, bool createParents = true) const;
    Result<void> Copy(const std::filesystem::path& from, const std::filesystem::path& to, bool overwrite = false) const;
    Result<void> Move(const std::filesystem::path& from, const std::filesystem::path& to, bool overwrite = false) const;
    Result<void> Remove(const std::filesystem::path& path) const;
};
}
