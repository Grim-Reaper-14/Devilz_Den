#pragma once

#include "Backend/Error/Result.hpp"
#include "Process_Info.hpp"

#include <string_view>
#include <vector>

namespace Devilz::Backend
{
class Process_Manager final
{
public:
    [[nodiscard]] Result<Process_Info> Current() const;
    [[nodiscard]] Result<std::vector<Process_Info>> Enumerate() const;
    [[nodiscard]] Result<Process_Info> Find(std::string_view executableName) const;
    [[nodiscard]] Result<bool> IsRunning(std::string_view executableName) const;

private:
    [[nodiscard]] Result<Process_Info> Inspect(std::uint32_t pid, std::string executableName) const;
};
}
