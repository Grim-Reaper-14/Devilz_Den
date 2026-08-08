#include "Memory_Manager.hpp"

#include "Backend/Logging/Logger_Manager.hpp"

#include <algorithm>
#include <sstream>
#include <thread>

namespace Devilz::Backend
{
Result<void> Memory_Manager::Initialize()
{
    m_state.store(ServiceState::Initializing, std::memory_order_release);
    ResetStatistics();
    m_state.store(ServiceState::Created, std::memory_order_release);
    return Result<void>::Success();
}

Result<void> Memory_Manager::Start()
{
    if (m_state.load(std::memory_order_acquire) == ServiceState::Failed)
        return Result<void>::Failure(Error(ErrorCode::ServiceInitializationFailed, ErrorCategory::Service,
            "Memory_Manager cannot start from failed state"));

    m_state.store(ServiceState::Running, std::memory_order_release);
    return Result<void>::Success();
}

Result<void> Memory_Manager::Stop()
{
    m_state.store(ServiceState::Stopping, std::memory_order_release);

    if (LeakDetectionEnabled()) {
        const auto leaks = FindLeaks();
        if (!leaks.empty()) {
            Logger_Manager::Instance().Warning(
                "Memory manager detected " + std::to_string(leaks.size()) + " active allocation(s) during stop",
                "Memory_Manager");
        }
    }

    m_state.store(ServiceState::Stopped, std::memory_order_release);
    return Result<void>::Success();
}

void Memory_Manager::Shutdown() noexcept
{
    try {
        if (LeakDetectionEnabled()) {
            const auto leaks = FindLeaks();
            if (!leaks.empty()) {
                Logger_Manager::Instance().Warning(
                    "Memory manager shutdown with " + std::to_string(leaks.size()) + " tracked allocation(s)",
                    "Memory_Manager");
            }
        }
    } catch (...) {
    }

    m_state.store(ServiceState::Stopped, std::memory_order_release);
}

void Memory_Manager::RegisterAllocation(const void* address,
                                        std::size_t size,
                                        MemoryTag tag,
                                        std::string owner,
                                        std::size_t alignment,
                                        std::source_location source)
{
    if (!TrackingEnabled() || address == nullptr || size == 0)
        return;

    MemoryAllocationRecord record;
    record.address = reinterpret_cast<std::uintptr_t>(address);
    record.size = size;
    record.alignment = alignment;
    record.tag = tag;
    record.owner = std::move(owner);
    record.threadName = CurrentThreadName();
    record.allocationId = m_nextAllocationId.fetch_add(1, std::memory_order_relaxed) + 1;
    record.source = source;

    std::scoped_lock lock(m_mutex);

    if (const auto existing = m_allocations.find(record.address); existing != m_allocations.end()) {
        const auto oldSize = existing->second.size;
        m_bytesInUse.fetch_sub(oldSize, std::memory_order_relaxed);
        m_activeAllocationCount.fetch_sub(1, std::memory_order_relaxed);

        auto& oldStats = m_tagStatistics[existing->second.tag];
        oldStats.bytesInUse = oldStats.bytesInUse >= oldSize ? oldStats.bytesInUse - oldSize : 0;
    }

    m_allocations.insert_or_assign(record.address, record);

    m_activeAllocationCount.fetch_add(1, std::memory_order_relaxed);
    m_totalAllocationCount.fetch_add(1, std::memory_order_relaxed);
    m_totalBytesAllocated.fetch_add(size, std::memory_order_relaxed);
    const auto current = m_bytesInUse.fetch_add(size, std::memory_order_relaxed) + size;
    UpdatePeak(current);

    auto& stats = m_tagStatistics[tag];
    stats.tag = tag;
    ++stats.allocationCount;
    stats.bytesInUse += size;
    stats.totalBytesAllocated += size;
    stats.peakBytesInUse = std::max(stats.peakBytesInUse, stats.bytesInUse);
}

void Memory_Manager::RegisterDeallocation(const void* address) noexcept
{
    if (!TrackingEnabled() || address == nullptr)
        return;

    try {
        const auto key = reinterpret_cast<std::uintptr_t>(address);
        std::scoped_lock lock(m_mutex);
        const auto it = m_allocations.find(key);
        if (it == m_allocations.end())
            return;

        const auto size = it->second.size;
        const auto tag = it->second.tag;
        m_allocations.erase(it);

        m_activeAllocationCount.fetch_sub(1, std::memory_order_relaxed);
        m_totalDeallocationCount.fetch_add(1, std::memory_order_relaxed);
        m_bytesInUse.fetch_sub(size, std::memory_order_relaxed);

        auto& stats = m_tagStatistics[tag];
        stats.tag = tag;
        ++stats.deallocationCount;
        stats.bytesInUse = stats.bytesInUse >= size ? stats.bytesInUse - size : 0;
    } catch (...) {
    }
}

MemorySnapshot Memory_Manager::Snapshot(bool includeActiveAllocations) const
{
    MemorySnapshot snapshot;
    snapshot.activeAllocationCount = m_activeAllocationCount.load(std::memory_order_acquire);
    snapshot.totalAllocationCount = m_totalAllocationCount.load(std::memory_order_acquire);
    snapshot.totalDeallocationCount = m_totalDeallocationCount.load(std::memory_order_acquire);
    snapshot.bytesInUse = m_bytesInUse.load(std::memory_order_acquire);
    snapshot.peakBytesInUse = m_peakBytesInUse.load(std::memory_order_acquire);
    snapshot.totalBytesAllocated = m_totalBytesAllocated.load(std::memory_order_acquire);

    std::scoped_lock lock(m_mutex);
    snapshot.tags.reserve(m_tagStatistics.size());
    for (const auto& [tag, stats] : m_tagStatistics) {
        (void)tag;
        snapshot.tags.push_back(stats);
    }

    if (includeActiveAllocations) {
        snapshot.activeAllocations.reserve(m_allocations.size());
        for (const auto& [address, record] : m_allocations) {
            (void)address;
            snapshot.activeAllocations.push_back(record);
        }
    }

    return snapshot;
}

std::vector<MemoryAllocationRecord> Memory_Manager::FindLeaks() const
{
    std::scoped_lock lock(m_mutex);
    std::vector<MemoryAllocationRecord> leaks;
    leaks.reserve(m_allocations.size());
    for (const auto& [address, record] : m_allocations) {
        (void)address;
        leaks.push_back(record);
    }
    return leaks;
}

void Memory_Manager::ResetStatistics()
{
    std::scoped_lock lock(m_mutex);
    m_allocations.clear();
    m_tagStatistics.clear();
    m_nextAllocationId.store(0, std::memory_order_release);
    m_activeAllocationCount.store(0, std::memory_order_release);
    m_totalAllocationCount.store(0, std::memory_order_release);
    m_totalDeallocationCount.store(0, std::memory_order_release);
    m_bytesInUse.store(0, std::memory_order_release);
    m_peakBytesInUse.store(0, std::memory_order_release);
    m_totalBytesAllocated.store(0, std::memory_order_release);
}

std::string Memory_Manager::CurrentThreadName()
{
    std::ostringstream stream;
    stream << std::this_thread::get_id();
    return stream.str();
}

void Memory_Manager::UpdatePeak(std::size_t current) noexcept
{
    auto peak = m_peakBytesInUse.load(std::memory_order_relaxed);
    while (current > peak &&
           !m_peakBytesInUse.compare_exchange_weak(peak, current,
                                                   std::memory_order_release,
                                                   std::memory_order_relaxed)) {
    }
}
}
