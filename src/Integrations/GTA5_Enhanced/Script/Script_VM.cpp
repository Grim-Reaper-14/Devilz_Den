#include "Script_VM.hpp"

#include <mutex>

namespace Devilz::Integrations::GTA5_Enhanced
{
using namespace Devilz::Backend;

Result<void> Script_VM::RegisterThread(scrThread* thread)
{
    if (!thread || !thread->Valid())
        return Result<void>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
            "Cannot register an invalid scrThread"));
    std::unique_lock lock(m_mutex);
    m_threads.insert_or_assign(thread->context.threadId, thread);
    return Result<void>::Success();
}

void Script_VM::UnregisterThread(std::uint32_t threadId) noexcept
{
    std::unique_lock lock(m_mutex);
    m_threads.erase(threadId);
}

void Script_VM::ClearThreads() noexcept
{
    std::unique_lock lock(m_mutex);
    m_threads.clear();
}

scrThread* Script_VM::FindThread(std::uint32_t threadId) const noexcept
{
    std::shared_lock lock(m_mutex);
    const auto it = m_threads.find(threadId);
    return it == m_threads.end() ? nullptr : it->second;
}

std::vector<scrThread*> Script_VM::Threads() const
{
    std::shared_lock lock(m_mutex);
    std::vector<scrThread*> result;
    result.reserve(m_threads.size());
    for (const auto& [id, thread] : m_threads) { (void)id; result.push_back(thread); }
    return result;
}

std::vector<scrThread*> Script_VM::RunningThreads() const
{
    std::shared_lock lock(m_mutex);
    std::vector<scrThread*> result;
    for (const auto& [id, thread] : m_threads) {
        (void)id;
        if (thread && thread->Running()) result.push_back(thread);
    }
    return result;
}

std::size_t Script_VM::ThreadCount() const noexcept
{
    std::shared_lock lock(m_mutex);
    return m_threads.size();
}
}
