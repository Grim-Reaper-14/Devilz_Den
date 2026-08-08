#pragma once

#include "Native_Registration.hpp"

#include <optional>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace Devilz::Integrations::GTA5_Enhanced
{
class Native_Cache final
{
public:
    void Store(Native_Registration registration);
    [[nodiscard]] std::optional<Native_Registration> Find(Native_Hash hash) const;
    [[nodiscard]] bool Contains(Native_Hash hash) const;
    void Invalidate(Native_Hash hash);
    void Clear();
    [[nodiscard]] std::size_t Size() const noexcept;
    [[nodiscard]] std::vector<Native_Registration> Snapshot() const;

private:
    mutable std::shared_mutex m_mutex;
    std::unordered_map<Native_Hash, Native_Registration> m_entries;
};
}
