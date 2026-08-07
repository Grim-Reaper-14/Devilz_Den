#pragma once

#include "Backend/Error/Result.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace Devilz::Backend
{
class Pattern
{
public:
    struct Token
    {
        std::uint8_t value = 0;
        bool wildcard = false;
    };

    static Result<Pattern> Compile(std::string_view signature);

    [[nodiscard]] const std::vector<Token>& Tokens() const noexcept { return m_tokens; }
    [[nodiscard]] std::size_t Size() const noexcept { return m_tokens.size(); }
    [[nodiscard]] bool Empty() const noexcept { return m_tokens.empty(); }
    [[nodiscard]] std::string_view Source() const noexcept { return m_source; }

    // Longest exact byte run, useful as a scan anchor before full verification.
    [[nodiscard]] std::size_t AnchorOffset() const noexcept { return m_anchorOffset; }
    [[nodiscard]] std::size_t AnchorLength() const noexcept { return m_anchorLength; }

private:
    std::string m_source;
    std::vector<Token> m_tokens;
    std::size_t m_anchorOffset = 0;
    std::size_t m_anchorLength = 0;
};
}
