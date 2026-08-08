#include "Native_Table.hpp"

#include <mutex>

namespace Devilz::Integrations::GTA5_Enhanced
{
using namespace Devilz::Backend;

Result<void> Native_Table::Register(Native_Registration registration)
{
    if (registration.hash.Value() == 0 || registration.handler.IsNull())
        return Result<void>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
            "Native registration contains an invalid hash or handler"));

    std::unique_lock lock(m_mutex);
    const auto it = m_entries.find(registration.hash);
    if (it != m_entries.end() && it->second.handler != registration.handler)
        return Result<void>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
            "Native table hash collision").With("Hash", std::to_string(registration.hash.Value())));

    m_entries.insert_or_assign(registration.hash, std::move(registration));
    return Result<void>::Success();
}

Result<void> Native_Table::RegisterBatch(std::vector<Native_Registration> registrations)
{
    for (auto& registration : registrations) {
        auto result = Register(std::move(registration));
        if (!result) return result;
    }
    return Result<void>::Success();
}

std::optional<Native_Registration> Native_Table::Find(Native_Hash runtimeHash) const
{
    std::shared_lock lock(m_mutex);
    const auto it = m_entries.find(runtimeHash);
    return it == m_entries.end() ? std::nullopt : std::optional<Native_Registration>(it->second);
}

bool Native_Table::Contains(Native_Hash runtimeHash) const
{
    std::shared_lock lock(m_mutex);
    return m_entries.contains(runtimeHash);
}

std::size_t Native_Table::Size() const noexcept
{
    std::shared_lock lock(m_mutex);
    return m_entries.size();
}

std::vector<Native_Registration> Native_Table::Snapshot() const
{
    std::shared_lock lock(m_mutex);
    std::vector<Native_Registration> result;
    result.reserve(m_entries.size());
    for (const auto& [hash, registration] : m_entries) { (void)hash; result.push_back(registration); }
    return result;
}

void Native_Table::Clear()
{
    std::unique_lock lock(m_mutex);
    m_entries.clear();
}
}
