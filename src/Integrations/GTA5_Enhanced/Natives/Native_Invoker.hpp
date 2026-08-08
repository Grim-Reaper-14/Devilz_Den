#pragma once

#include "Backend/Error/Result.hpp"
#include "Native_Call_Context.hpp"
#include "Native_Handler_Table.hpp"
#include "Native_Index.hpp"
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
    std::uint64_t indexedHits = 0;
};

class Native_Invoker final
{
public:
    Native_Invoker(Native_Manager& manager, Native_Table& table, Native_Handler_Table* indexedHandlers = nullptr) noexcept
        : m_manager(manager), m_table(table), m_indexedHandlers(indexedHandlers) {}

    Devilz::Backend::Result<void> Invoke(Native_Hash canonicalHash, Native_Call_Context& context);
    Devilz::Backend::Result<void> Invoke(Native_Index index, Native_Call_Context& context);

    void SetIndexedHandlers(Native_Handler_Table* table) noexcept { m_indexedHandlers = table; }
    [[nodiscard]] Native_Invoke_Stats Statistics() const noexcept { return m_stats; }

private:
    Devilz::Backend::Result<void> InvokeResolved(Devilz::Backend::Pointer handler, Native_Call_Context& context,
        Native_Hash canonicalHash = Native_Hash{});

    Native_Manager& m_manager;
    Native_Table& m_table;
    Native_Handler_Table* m_indexedHandlers = nullptr;
    Native_Invoke_Stats m_stats{};
};
}
