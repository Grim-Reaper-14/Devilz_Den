#pragma once

#include "Process_Memory_Reader.hpp"
#include "Process_Module_Info.hpp"
#include "Process_Pattern.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace Devilz::Backend
{
struct Process_Pattern_Match
{
    std::uintptr_t address = 0;
    std::size_t offset = 0;
};

class Process_Pattern_Scanner final
{
public:
    explicit Process_Pattern_Scanner(std::uint32_t pid) noexcept : m_reader(pid) {}

    [[nodiscard]] Result<std::vector<Process_Pattern_Match>> Scan(
        const Process_Module_Info& module,
        const Process_Pattern& pattern,
        std::size_t maxMatches = 16) const;

private:
    Process_Memory_Reader m_reader;
};
}
