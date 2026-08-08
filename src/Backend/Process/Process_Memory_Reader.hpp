#pragma once

#include "Backend/Error/Result.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace Devilz::Backend
{
class Process_Memory_Reader final
{
public:
    explicit Process_Memory_Reader(std::uint32_t pid) noexcept : m_pid(pid) {}

    [[nodiscard]] std::uint32_t Pid() const noexcept { return m_pid; }
    [[nodiscard]] Result<std::vector<std::byte>> Read(std::uintptr_t address, std::size_t size) const;
    [[nodiscard]] Result<void> ReadInto(std::uintptr_t address, std::span<std::byte> destination) const;

private:
    std::uint32_t m_pid = 0;
};
}
