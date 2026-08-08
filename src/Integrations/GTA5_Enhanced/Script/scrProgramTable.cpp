#include "scrProgramTable.hpp"

#include <mutex>

namespace Devilz::Integrations::GTA5_Enhanced
{
using namespace Devilz::Backend;

Result<void> scrProgramTable::Register(scrProgram* program)
{
    if (!program || !program->Valid())
        return Result<void>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
            "Cannot register an invalid scrProgram"));

    std::unique_lock lock(m_mutex);
    m_programs.insert_or_assign(program->nameHash, program);
    return Result<void>::Success();
}

void scrProgramTable::Unregister(std::uint32_t hash) noexcept
{
    std::unique_lock lock(m_mutex);
    m_programs.erase(hash);
}

scrProgram* scrProgramTable::Find(std::uint32_t hash) const noexcept
{
    std::shared_lock lock(m_mutex);
    const auto it = m_programs.find(hash);
    return it == m_programs.end() ? nullptr : it->second;
}

bool scrProgramTable::Contains(std::uint32_t hash) const noexcept
{
    std::shared_lock lock(m_mutex);
    return m_programs.contains(hash);
}

std::vector<scrProgram*> scrProgramTable::Snapshot() const
{
    std::shared_lock lock(m_mutex);
    std::vector<scrProgram*> result;
    result.reserve(m_programs.size());
    for (const auto& [hash, program] : m_programs) { (void)hash; result.push_back(program); }
    return result;
}

void scrProgramTable::Clear() noexcept
{
    std::unique_lock lock(m_mutex);
    m_programs.clear();
}

std::size_t scrProgramTable::Size() const noexcept
{
    std::shared_lock lock(m_mutex);
    return m_programs.size();
}
}
