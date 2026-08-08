#pragma once

#include "Backend/Error/Result.hpp"
#include "scrProgramTable.hpp"
#include "scrThread.hpp"

#include <cstddef>
#include <cstdint>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace Devilz::Integrations::GTA5_Enhanced
{
class Script_VM final
{
public:
    Devilz::Backend::Result<void> RegisterThread(scrThread* thread);
    void UnregisterThread(std::uint32_t threadId) noexcept;
    void ClearThreads() noexcept;

    [[nodiscard]] scrThread* FindThread(std::uint32_t threadId) const noexcept;
    [[nodiscard]] std::vector<scrThread*> Threads() const;
    [[nodiscard]] std::vector<scrThread*> RunningThreads() const;
    [[nodiscard]] std::size_t ThreadCount() const noexcept;

private:
    mutable std::shared_mutex m_mutex;
    std::unordered_map<std::uint32_t, scrThread*> m_threads;
};
}
