#include "Script_Global_Manager.hpp"

#include <algorithm>
#include <cstring>
#include <limits>
#include <mutex>
#include <utility>

namespace Devilz::Integrations::GTA5_Enhanced
{
using namespace Devilz::Backend;

Result<void> Script_Global_Manager::Configure(std::vector<Block> blocks, std::uint64_t buildFingerprint)
{
    if (buildFingerprint == 0)
        return Result<void>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
            "Script globals require a valid build fingerprint"));

    for (const auto& block : blocks) {
        if (block.slotCount == 0 || block.base.IsNull())
            return Result<void>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
                "Script global block is invalid"));
        if (block.firstIndex > std::numeric_limits<std::uint32_t>::max() - block.slotCount)
            return Result<void>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
                "Script global block index range overflows"));
    }

    std::sort(blocks.begin(), blocks.end(), [](const Block& a, const Block& b) { return a.firstIndex < b.firstIndex; });
    for (std::size_t i = 1; i < blocks.size(); ++i) {
        if (blocks[i - 1].firstIndex + blocks[i - 1].slotCount > blocks[i].firstIndex)
            return Result<void>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
                "Script global blocks overlap"));
    }

    std::unique_lock lock(m_mutex);
    m_blocks = std::move(blocks);
    m_buildFingerprint = buildFingerprint;
    return Result<void>::Success();
}

Result<void> Script_Global_Manager::ConfigureFromTable(
    Pointer table,
    std::uint64_t buildFingerprint)
{
    constexpr std::uint32_t BlockCount = 64U;
    constexpr std::uint32_t BlockShift = 18U;
    constexpr std::uint32_t SlotsPerBlock = 1U << BlockShift;

    if (table.IsNull())
        return Result<void>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
            "Script global table is null"));
    if (buildFingerprint == 0)
        return Result<void>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
            "Script global table requires a valid build fingerprint"));

    std::vector<Block> blocks;
    blocks.reserve(BlockCount);

    for (std::uint32_t blockIndex = 0; blockIndex < BlockCount; ++blockIndex) {
        std::uintptr_t blockAddress = 0;
        const auto entryAddress = table.Add(
            static_cast<std::ptrdiff_t>(blockIndex * sizeof(std::uintptr_t)));
        std::memcpy(
            &blockAddress,
            reinterpret_cast<const void*>(entryAddress.Address()),
            sizeof(blockAddress));

        if (blockAddress == 0)
            continue;

        Block block{};
        block.firstIndex = blockIndex << BlockShift;
        block.slotCount = SlotsPerBlock;
        block.base = Pointer(blockAddress);
        blocks.push_back(block);
    }

    if (blocks.empty())
        return Result<void>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
            "Script global table contains no available blocks"));

    return Configure(std::move(blocks), buildFingerprint);
}

void Script_Global_Manager::Clear()
{
    std::unique_lock lock(m_mutex);
    m_blocks.clear();
    m_buildFingerprint = 0;
}

Result<Pointer> Script_Global_Manager::Resolve(std::uint32_t index) const
{
    std::shared_lock lock(m_mutex);
    if (m_buildFingerprint == 0)
        return Result<Pointer>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
            "Script global manager is not configured"));

    const auto it = std::find_if(m_blocks.begin(), m_blocks.end(), [index](const Block& block) { return block.Contains(index); });
    if (it == m_blocks.end())
        return Result<Pointer>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
            "Script global index is outside configured blocks").With("Index", std::to_string(index)));

    constexpr std::size_t SlotSize = sizeof(std::uint64_t);
    const auto local = static_cast<std::size_t>(index - it->firstIndex);
    return Result<Pointer>::Success(it->base.Add(static_cast<std::ptrdiff_t>(local * SlotSize)));
}

bool Script_Global_Manager::Ready() const noexcept
{
    std::shared_lock lock(m_mutex);
    return m_buildFingerprint != 0 && !m_blocks.empty();
}

std::uint64_t Script_Global_Manager::BuildFingerprint() const noexcept
{
    std::shared_lock lock(m_mutex);
    return m_buildFingerprint;
}

Script_Global Script_Global::At(std::ptrdiff_t offset) const noexcept
{
    if (offset < 0 && static_cast<std::uint64_t>(-offset) > m_index) return {};
    return Script_Global(m_manager, static_cast<std::uint32_t>(static_cast<std::int64_t>(m_index) + offset));
}

Result<Pointer> Script_Global::Resolve() const
{
    if (!m_manager)
        return Result<Pointer>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
            "Script global has no manager"));
    return m_manager->Resolve(m_index);
}
}
