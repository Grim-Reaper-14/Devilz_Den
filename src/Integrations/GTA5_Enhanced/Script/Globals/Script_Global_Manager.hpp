#pragma once

#include "Backend/Error/Result.hpp"
#include "Backend/Memory/Pointer.hpp"
#include "Script_Global.hpp"

#include <cstddef>
#include <cstdint>
#include <shared_mutex>
#include <vector>

namespace Devilz::Integrations::GTA5_Enhanced
{
class Script_Global_Manager final
{
public:
    struct Block
    {
        std::uint32_t firstIndex = 0;
        std::uint32_t slotCount = 0;
        Devilz::Backend::Pointer base;

        [[nodiscard]] bool Contains(std::uint32_t index) const noexcept
        {
            return index >= firstIndex && index - firstIndex < slotCount;
        }
    };

    Devilz::Backend::Result<void> Configure(std::vector<Block> blocks,
                                            std::uint64_t buildFingerprint);
    void Clear();

    [[nodiscard]] Script_Global Get(std::uint32_t index) noexcept { return Script_Global(this, index); }
    [[nodiscard]] Devilz::Backend::Result<Devilz::Backend::Pointer> Resolve(std::uint32_t index) const;
    [[nodiscard]] bool Ready() const noexcept;
    [[nodiscard]] std::uint64_t BuildFingerprint() const noexcept;

private:
    mutable std::shared_mutex m_mutex;
    std::vector<Block> m_blocks;
    std::uint64_t m_buildFingerprint = 0;
};
}
