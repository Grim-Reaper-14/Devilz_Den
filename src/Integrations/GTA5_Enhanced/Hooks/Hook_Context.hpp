#pragma once
#include <chrono>
#include <cstdint>

namespace Devilz::Integrations::GTA5_Enhanced {
struct Hook_Context {
    std::uint64_t callCount = 0;
    std::uint64_t failureCount = 0;
    std::chrono::nanoseconds totalExecutionTime{};
};
}
