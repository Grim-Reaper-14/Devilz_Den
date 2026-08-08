#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace Devilz::Backend
{
enum class Process_Architecture : std::uint8_t
{
    Unknown,
    X86,
    X64,
    Arm64
};

struct Process_Info
{
    std::uint32_t pid = 0;
    std::string executableName;
    std::filesystem::path executablePath;
    Process_Architecture architecture = Process_Architecture::Unknown;
    bool running = false;

    [[nodiscard]] bool Valid() const noexcept
    {
        return pid != 0 && running && !executableName.empty();
    }
};
}
