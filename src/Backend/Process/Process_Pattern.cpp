#include "Process_Pattern.hpp"

#include <charconv>
#include <sstream>

namespace Devilz::Backend
{
std::optional<Process_Pattern> Process_Pattern::Parse(std::string_view text)
{
    Process_Pattern pattern;
    std::istringstream stream{std::string(text)};
    std::string token;

    while (stream >> token) {
        if (token == "?" || token == "??") {
            pattern.m_bytes.push_back({0, true});
            continue;
        }

        if (token.size() != 2)
            return std::nullopt;

        unsigned int value = 0;
        const auto result = std::from_chars(token.data(), token.data() + token.size(), value, 16);
        if (result.ec != std::errc{} || result.ptr != token.data() + token.size() || value > 0xFF)
            return std::nullopt;

        pattern.m_bytes.push_back({static_cast<std::uint8_t>(value), false});
    }

    if (pattern.Empty())
        return std::nullopt;
    return pattern;
}
}
