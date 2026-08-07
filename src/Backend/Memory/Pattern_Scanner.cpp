#include "Pattern_Scanner.hpp"

#include <algorithm>

namespace Devilz::Backend
{
bool Pattern_Scanner::Matches(const std::byte* candidate, const Pattern& pattern) noexcept
{
    const auto& tokens = pattern.Tokens();
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        if (!tokens[i].wildcard && std::to_integer<std::uint8_t>(candidate[i]) != tokens[i].value)
            return false;
    }
    return true;
}

Result<std::vector<Pattern_Scan_Result>> Pattern_Scanner::Scan(const Memory_Range& range,
                                                               const Pattern& pattern,
                                                               Options options)
{
    const auto started = std::chrono::steady_clock::now();
    m_lastStats = {};

    if (range.Empty() || pattern.Empty() || pattern.Size() > range.size)
        return Result<std::vector<Pattern_Scan_Result>>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime, "Invalid pattern scan range or pattern"));

    if (options.maxResults == 0)
        options.maxResults = static_cast<std::size_t>(-1);

    std::vector<Pattern_Scan_Result> matches;
    const auto bytes = range.Bytes();
    const auto limit = bytes.size() - pattern.Size() + 1;
    const auto& tokens = pattern.Tokens();
    const auto anchorOffset = pattern.AnchorOffset();
    const auto anchorLength = pattern.AnchorLength();

    for (std::size_t offset = 0; offset < limit && matches.size() < options.maxResults; ++offset) {
        ++m_lastStats.candidateCount;

        bool anchorMatches = true;
        for (std::size_t a = 0; a < anchorLength; ++a) {
            if (std::to_integer<std::uint8_t>(bytes[offset + anchorOffset + a]) != tokens[anchorOffset + a].value) {
                anchorMatches = false;
                break;
            }
        }
        if (!anchorMatches) continue;

        if (Matches(bytes.data() + offset, pattern))
            matches.push_back({Pointer(range.begin + offset), offset});
    }

    m_lastStats.bytesScanned = range.size;
    m_lastStats.matchCount = matches.size();
    m_lastStats.elapsed = std::chrono::steady_clock::now() - started;

    return Result<std::vector<Pattern_Scan_Result>>::Success(std::move(matches));
}

Result<Pattern_Scan_Result> Pattern_Scanner::FindFirst(const Memory_Range& range, const Pattern& pattern)
{
    auto result = Scan(range, pattern, Options{1, true});
    if (!result)
        return Result<Pattern_Scan_Result>::Failure(result.Failure());
    if (result.Value().empty())
        return Result<Pattern_Scan_Result>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime, "Pattern was not found").With("Pattern", std::string(pattern.Source())));
    return Result<Pattern_Scan_Result>::Success(result.Value().front());
}
}
