#pragma once

#include "Backend/Error/Result.hpp"
#include "Native_Registration.hpp"

#include <optional>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace Devilz::Integrations::GTA5_Enhanced
{
class Native_Table final
{
public:
    Devilz::Backend::Result<void> Register(Native_Registration registration);
    Devilz::Backend::Result<void> RegisterBatch(std::vector<Native_Registration> registrations);

    [[nodiscard]] std::optional<Native_Registration> Find(Native_Hash runtimeHash) const;
    [[nodiscard]] bool Contains(Native_Hash runtimeHash) const;
    [[nodiscard]] std::size_t Size() const noexcept;
    [[nodiscard]] std::vector<Native_Registration> Snapshot() const;
    void Clear();

private:
    mutable std::shared_mutex m_mutex;
    std::unordered_map<Native_Hash, Native_Registration> m_entries;
};
}
