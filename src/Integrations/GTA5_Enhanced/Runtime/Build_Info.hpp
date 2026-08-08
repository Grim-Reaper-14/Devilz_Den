#pragma once
#include <cstdint>
#include <filesystem>
#include <string>

namespace Devilz::Integrations::GTA5_Enhanced {
struct Build_Info {
    std::string version;
    std::uint32_t peTimestamp = 0;
    std::uint32_t imageSize = 0;
    std::uint64_t textHash = 0;
    std::uint64_t rdataHash = 0;
    std::uint64_t fingerprint = 0;
    std::filesystem::path executablePath;
};
}
