#include "Pattern.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>

namespace Devilz::Backend
{
Result<Pattern> Pattern::Compile(std::string_view signature)
{
    Pattern pattern;
    pattern.m_source.assign(signature);

    std::size_t i = 0;
    while (i < signature.size()) {
        while (i < signature.size() && std::isspace(static_cast<unsigned char>(signature[i]))) ++i;
        if (i == signature.size()) break;

        const auto start = i;
        while (i < signature.size() && !std::isspace(static_cast<unsigned char>(signature[i]))) ++i;
        const auto token = signature.substr(start, i - start);

        if (token == "?" || token == "??") {
            pattern.m_tokens.push_back({0, true});
            continue;
        }

        if (token.size() != 2)
            return Result<Pattern>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime, "Invalid pattern token").With("Token", std::string(token)).With("Pattern", std::string(signature)));

        unsigned value = 0;
        const auto result = std::from_chars(token.data(), token.data() + token.size(), value, 16);
        if (result.ec != std::errc{} || result.ptr != token.data() + token.size() || value > 0xFF)
            return Result<Pattern>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime, "Invalid hexadecimal pattern byte").With("Token", std::string(token)).With("Pattern", std::string(signature)));

        pattern.m_tokens.push_back({static_cast<std::uint8_t>(value), false});
    }

    if (pattern.m_tokens.empty())
        return Result<Pattern>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime, "Pattern cannot be empty"));

    std::size_t runStart = 0, runLength = 0;
    for (std::size_t index = 0; index <= pattern.m_tokens.size(); ++index) {
        if (index < pattern.m_tokens.size() && !pattern.m_tokens[index].wildcard) {
            if (runLength == 0) runStart = index;
            ++runLength;
        } else {
            if (runLength > pattern.m_anchorLength) {
                pattern.m_anchorOffset = runStart;
                pattern.m_anchorLength = runLength;
            }
            runLength = 0;
        }
    }

    return Result<Pattern>::Success(std::move(pattern));
}
}
