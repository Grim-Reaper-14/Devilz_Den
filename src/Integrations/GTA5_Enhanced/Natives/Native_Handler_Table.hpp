#pragma once

#include "Backend/Memory/Pointer.hpp"
#include "Native_Hash.hpp"
#include "Native_Index.hpp"

#include <cstddef>
#include <optional>
#include <vector>

namespace Devilz::Integrations::GTA5_Enhanced
{
class Native_Handler_Table final
{
public:
    using Pointer = Devilz::Backend::Pointer;

    void Reset(std::size_t count)
    {
        m_hashes.assign(count, Native_Hash{});
        m_handlers.assign(count, Pointer{});
        m_validated.assign(count, false);
    }

    void Clear() noexcept
    {
        m_hashes.clear();
        m_handlers.clear();
        m_validated.clear();
    }

    [[nodiscard]] std::size_t Size() const noexcept { return m_handlers.size(); }

    [[nodiscard]] bool Set(Native_Index index, Native_Hash runtimeHash, Pointer handler, bool validated) noexcept
    {
        const auto i = index.AsSize();
        if (i >= m_handlers.size()) return false;
        m_hashes[i] = runtimeHash;
        m_handlers[i] = handler;
        m_validated[i] = validated;
        return true;
    }

    [[nodiscard]] std::optional<Pointer> Get(Native_Index index) const noexcept
    {
        const auto i = index.AsSize();
        if (i >= m_handlers.size() || !m_validated[i] || m_handlers[i].IsNull()) return std::nullopt;
        return m_handlers[i];
    }

    [[nodiscard]] std::optional<Native_Hash> RuntimeHash(Native_Index index) const noexcept
    {
        const auto i = index.AsSize();
        if (i >= m_hashes.size()) return std::nullopt;
        return m_hashes[i];
    }

private:
    std::vector<Native_Hash> m_hashes;
    std::vector<Pointer> m_handlers;
    std::vector<bool> m_validated;
};
}
