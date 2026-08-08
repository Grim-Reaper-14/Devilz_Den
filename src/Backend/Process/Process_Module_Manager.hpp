#pragma once

#include "Backend/Error/Result.hpp"
#include "Process_Module_Info.hpp"

#include <cstdint>
#include <string_view>
#include <vector>

namespace Devilz::Backend
{
class Process_Module_Manager final
{
public:
    explicit Process_Module_Manager(std::uint32_t pid) noexcept : m_pid(pid) {}

    [[nodiscard]] Result<std::vector<Process_Module_Info>> Enumerate() const;
    [[nodiscard]] Result<Process_Module_Info> Find(std::string_view moduleName) const;

private:
    std::uint32_t m_pid = 0;
};
}
