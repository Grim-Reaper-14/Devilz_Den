#pragma once
#include <cstdint>
#include <functional>

namespace Devilz::Integrations::GTA5_Enhanced {
class Native_Hash final {
public:
    constexpr Native_Hash() = default;
    constexpr explicit Native_Hash(std::uint64_t value) noexcept : m_value(value) {}
    [[nodiscard]] constexpr std::uint64_t Value() const noexcept { return m_value; }
    [[nodiscard]] constexpr bool operator==(const Native_Hash&) const noexcept = default;
private:
    std::uint64_t m_value = 0;
};
}

template<> struct std::hash<Devilz::Integrations::GTA5_Enhanced::Native_Hash> {
    std::size_t operator()(Devilz::Integrations::GTA5_Enhanced::Native_Hash hash) const noexcept { return std::hash<std::uint64_t>{}(hash.Value()); }
};
