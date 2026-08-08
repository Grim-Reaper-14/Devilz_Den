#pragma once

#include "Backend/Error/Result.hpp"
#include "scrProgram.hpp"

#include <cstddef>
#include <cstdint>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace Devilz::Integrations::GTA5_Enhanced
{
class scrProgramTable final
{
public:
    Devilz::Backend::Result<void> Register(scrProgram* program);
    void Unregister(std::uint32_t hash) noexcept;
    void Clear() noexcept;

    [[nodiscard]] scrProgram* Find(std::uint32_t hash) const noexcept;
    [[nodiscard]] bool Contains(std::uint32_t hash) const noexcept;
    [[nodiscard]] std::vector<scrProgram*> Snapshot() const;
    [[nodiscard]] std::size_t Size() const noexcept;

private:
    mutable std::shared_mutex m_mutex;
    std::unordered_map<std::uint32_t, scrProgram*> m_programs;
};
}
