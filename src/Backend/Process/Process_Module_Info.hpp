#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace Devilz::Backend
{
struct Process_Module_Info
{
    std::string name;
    std::filesystem::path path;
    std::uintptr_t baseAddress = 0;
    std::uint32_t imageSize = 0;

    [[nodiscard]] bool Valid() const noexcept
    {
        return !name.empty() && baseAddress != 0 && imageSize != 0;
    }
};
}
