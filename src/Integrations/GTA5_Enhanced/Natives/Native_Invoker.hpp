#pragma once

#include "Backend/Error/Result.hpp"
#include "Native_Call_Context.hpp"
#include "Native_Manager.hpp"
#include "Native_Table.hpp"

#include <cstdint>

namespace Devilz::Integrations::GTA5_Enhanced
{
struct Native_Invoke_Stats
{
    std::uint64_t calls = 0;
    std::uint64_t failures = 0;
    std::uint64_t cacheHits = 0;
    std::uint64_t tableHits = 0;
};

class Native_Invoker final
{
public:
    Native_Invoker(Native_Manager& manager, Native_Table& table) noexcept
        : m_manager(manager), m_table(table) {}

    Devilz::Backend::Result<void> Invoke(Native_Hash canonicalHash, Native_Call_Context& context);
    [[nodiscard]] Native_Invoke_Stats Statistics() const noexcept { return m_stats; }

private:
    Native_Manager& m_manager;
    Native_Table& m_table;
    Native_Invoke_Stats m_stats{};
};
}
