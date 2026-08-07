#pragma once

#include "Backend/Error/Result.hpp"
#include "Backend/Services/IBackendService.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <source_location>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Devilz::Backend
{
enum class MemoryTag : std::uint16_t
{
    Unknown = 0,
    Core,
    Backend,
    Logging,
    Threading,
    Filesystem,
    Graphics,
    Resources,
    Input,
    Network,
    Configuration,
    Scripting,
    Frontend,
    Temporary
};

struct MemoryAllocationRecord
{
    std::uintptr_t address = 0;
    std::size_t size = 0;
    std::size_t alignment = alignof(std::max_align_t);
    MemoryTag tag = MemoryTag::Unknown;
    std::string owner;
    std::string threadName;
    std::uint64_t allocationId = 0;
    std::source_location source = std::source_location::current();
};

struct MemoryTagStatistics
{
    MemoryTag tag = MemoryTag::Unknown;
    std::uint64_t allocationCount = 0;
    std::uint64_t deallocationCount = 0;
    std::size_t bytesInUse = 0;
    std::size_t peakBytesInUse = 0;
    std::size_t totalBytesAllocated = 0;
};

struct MemorySnapshot
{
    std::uint64_t activeAllocationCount = 0;
    std::uint64_t totalAllocationCount = 0;
    std::uint64_t totalDeallocationCount = 0;
    std::size_t bytesInUse = 0;
    std::size_t peakBytesInUse = 0;
    std::size_t totalBytesAllocated = 0;
    std::vector<MemoryTagStatistics> tags;
    std::vector<MemoryAllocationRecord> activeAllocations;
};

class Memory_Manager final : public IBackendService
{
public:
    static Memory_Manager& Instance() noexcept
    {
        static Memory_Manager instance;
        return instance;
    }

    Memory_Manager(const Memory_Manager&) = delete;
    Memory_Manager& operator=(const Memory_Manager&) = delete;
    Memory_Manager(Memory_Manager&&) = delete;
    Memory_Manager& operator=(Memory_Manager&&) = delete;

    [[nodiscard]] std::string_view Name() const noexcept override
    {
        return "Memory_Manager";
    }

    [[nodiscard]] ServiceState State() const noexcept override
    {
        return m_state.load(std::memory_order_acquire);
    }

    Result<void> Initialize() override;
    Result<void> Start() override;
    Result<void> Stop() override;
    void Shutdown() noexcept override;

    void SetTrackingEnabled(bool enabled) noexcept
    {
        m_trackingEnabled.store(enabled, std::memory_order_release);
    }

    [[nodiscard]] bool TrackingEnabled() const noexcept
    {
        return m_trackingEnabled.load(std::memory_order_acquire);
    }

    void SetLeakDetectionEnabled(bool enabled) noexcept
    {
        m_leakDetectionEnabled.store(enabled, std::memory_order_release);
    }

    [[nodiscard]] bool LeakDetectionEnabled() const noexcept
    {
        return m_leakDetectionEnabled.load(std::memory_order_acquire);
    }

    void RegisterAllocation(
        const void* address,
        std::size_t size,
        MemoryTag tag = MemoryTag::Unknown,
        std::string owner = {},
        std::size_t alignment = alignof(std::max_align_t),
        std::source_location source = std::source_location::current());

    void RegisterDeallocation(const void* address) noexcept;

    [[nodiscard]] MemorySnapshot Snapshot(bool includeActiveAllocations = false) const;
    [[nodiscard]] std::vector<MemoryAllocationRecord> FindLeaks() const;
    [[nodiscard]] std::size_t BytesInUse() const noexcept
    {
        return m_bytesInUse.load(std::memory_order_acquire);
    }

    [[nodiscard]] std::size_t PeakBytesInUse() const noexcept
    {
        return m_peakBytesInUse.load(std::memory_order_acquire);
    }

    [[nodiscard]] std::uint64_t ActiveAllocationCount() const noexcept
    {
        return m_activeAllocationCount.load(std::memory_order_acquire);
    }

    void ResetStatistics();

    template <typename T, typename... Args>
    std::unique_ptr<T> MakeUnique(
        MemoryTag tag,
        std::string owner,
        Args&&... args)
    {
        auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
        RegisterAllocation(ptr.get(), sizeof(T), tag, std::move(owner), alignof(T));
        return ptr;
    }

private:
    Memory_Manager() = default;
    ~Memory_Manager() override = default;

    static std::string CurrentThreadName();
    void UpdatePeak(std::size_t current) noexcept;

    std::atomic<ServiceState> m_state{ServiceState::Created};
    std::atomic_bool m_trackingEnabled{true};
    std::atomic_bool m_leakDetectionEnabled{true};

    std::atomic<std::uint64_t> m_nextAllocationId{0};
    std::atomic<std::uint64_t> m_activeAllocationCount{0};
    std::atomic<std::uint64_t> m_totalAllocationCount{0};
    std::atomic<std::uint64_t> m_totalDeallocationCount{0};
    std::atomic<std::size_t> m_bytesInUse{0};
    std::atomic<std::size_t> m_peakBytesInUse{0};
    std::atomic<std::size_t> m_totalBytesAllocated{0};

    mutable std::mutex m_mutex;
    std::unordered_map<std::uintptr_t, MemoryAllocationRecord> m_allocations;
    std::unordered_map<MemoryTag, MemoryTagStatistics> m_tagStatistics;
};
}
