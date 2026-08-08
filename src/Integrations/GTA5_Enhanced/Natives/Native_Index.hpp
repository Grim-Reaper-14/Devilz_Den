#pragma once

#include <cstddef>
#include <cstdint>

namespace Devilz::Integrations::GTA5_Enhanced
{
class Native_Index final
{
public:
    constexpr Native_Index() noexcept = default;
    constexpr explicit Native_Index(std::uint32_t value) noexcept : m_value(value) {}

    [[nodiscard]] constexpr std::uint32_t Value() const noexcept { return m_value; }
    [[nodiscard]] constexpr std::size_t AsSize() const noexcept { return static_cast<std::size_t>(m_value); }
    [[nodiscard]] constexpr bool operator==(const Native_Index&) const noexcept = default;

private:
    std::uint32_t m_value = 0;
};
}
