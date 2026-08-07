#pragma once

#include <cstdint>
#include <functional>
#include <string_view>

namespace Devilz::Backend
{
using TaskId = std::uint64_t;
using Task = std::function<void()>;

class IExecutor
{
public:
    virtual ~IExecutor() = default;
    [[nodiscard]] virtual std::string_view Name() const noexcept = 0;
    virtual TaskId Submit(Task task) = 0;
    [[nodiscard]] virtual std::size_t Pending() const noexcept = 0;
};
}
