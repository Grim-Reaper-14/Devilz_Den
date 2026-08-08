#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Devilz::Backend
{
struct Process_Pattern_Byte
{
    std::uint8_t value = 0;
    bool wildcard = false;
};

class Process_Pattern final
{
public:
    [[nodiscard]] static std::optional<Process_Pattern> Parse(std::string_view text);

    [[nodiscard]] const std::vector<Process_Pattern_Byte>& Bytes() const noexcept { return m_bytes; }
    [[nodiscard]] std::size_t Size() const noexcept { return m_bytes.size(); }
    [[nodiscard]] bool Empty() const noexcept { return m_bytes.empty(); }

private:
    std::vector<Process_Pattern_Byte> m_bytes;
};
}
