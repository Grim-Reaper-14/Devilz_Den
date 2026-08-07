#pragma once

#include "Memory_Range.hpp"
#include "Pattern.hpp"
#include "Pointer.hpp"

#include <chrono>
#include <cstddef>
#include <vector>

namespace Devilz::Backend
{
struct Pattern_Scan_Result
{
    Pointer address;
    std::size_t offset = 0;
};

struct Pattern_Scan_Stats
{
    std::size_t bytesScanned = 0;
    std::size_t candidateCount = 0;
    std::size_t matchCount = 0;
    std::chrono::nanoseconds elapsed{};
};

class Pattern_Scanner
{
public:
    struct Options
    {
        std::size_t maxResults = 1;
        bool collectStatistics = true;
    };

    [[nodiscard]] Result<std::vector<Pattern_Scan_Result>> Scan(
        const Memory_Range& range,
        const Pattern& pattern,
        Options options = {});

    [[nodiscard]] Result<Pattern_Scan_Result> FindFirst(
        const Memory_Range& range,
        const Pattern& pattern);

    [[nodiscard]] const Pattern_Scan_Stats& LastStatistics() const noexcept { return m_lastStats; }

private:
    static bool Matches(const std::byte* candidate, const Pattern& pattern) noexcept;
    Pattern_Scan_Stats m_lastStats{};
};
}
