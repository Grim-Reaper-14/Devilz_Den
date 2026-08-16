#include "GTA_Script_Function_Invoker.hpp"

#include <Windows.h>
#include <intrin.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cctype>
#include <cstring>
#include <deque>
#include <limits>
#include <mutex>
#include <optional>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Devilz::Integrations::GTA5_Enhanced
{
namespace
{
constexpr std::size_t ScriptProgramCount = 176;
constexpr std::size_t ScriptCodePageSize = 0x4000;
constexpr std::size_t MaxScriptThreads = 256;
constexpr std::size_t MinPatternBytes = 8;
constexpr std::size_t MinExactPatternBytes = 6;
constexpr std::size_t MaxPatternBytes = 96;
constexpr std::size_t MaxScriptStackSlots = 0x10000;
constexpr std::size_t MaxFunctionArguments = 32;
constexpr std::size_t MaxPendingRequests = 16;
constexpr std::size_t MaxRequestHistory = 64;

struct GTA_Tls_Context_View
{
    std::byte pad00[0x7A0]{};
    void* currentScriptThread = nullptr;
    bool scriptThreadActive = false;
};

static_assert(offsetof(GTA_Tls_Context_View, currentScriptThread) == 0x7A0);
static_assert(offsetof(GTA_Tls_Context_View, scriptThreadActive) == 0x7A8);

struct GTA_Script_Context_View
{
    std::uint32_t threadId = 0;
    std::uint32_t pad04 = 0;
    std::uint64_t scriptHash = 0;
    std::int32_t state = 0;
    std::uint32_t programCounter = 0;
    std::uint32_t framePointer = 0;
    std::uint32_t stackPointer = 0;
    float timerA = 0.0F;
    float timerB = 0.0F;
    float waitTimer = 0.0F;
    std::byte pad2C[0x2C]{};
    std::uint32_t stackSize = 0;
    std::byte pad5C[0x54]{};
};

static_assert(sizeof(GTA_Script_Context_View) == 0xB0);

struct GTA_Script_Thread_View
{
    void* vtable = nullptr;
    GTA_Script_Context_View context{};
    std::uint64_t* stack = nullptr;
    std::byte padC0[0x90]{};
    std::uint32_t scriptHash = 0;
};

static_assert(offsetof(GTA_Script_Thread_View, context) == 0x08);
static_assert(offsetof(GTA_Script_Thread_View, stack) == 0xB8);
static_assert(offsetof(GTA_Script_Thread_View, scriptHash) == 0x150);

struct GTA_Script_Thread_Array_View
{
    GTA_Script_Thread_View** data = nullptr;
    std::uint16_t size = 0;
    std::uint16_t capacity = 0;
    std::uint32_t pad0C = 0;
};

static_assert(sizeof(GTA_Script_Thread_Array_View) == 0x10);

struct GTA_Script_Program_View
{
    std::byte pad00[0x10]{};
    std::uint8_t** codeBlocks = nullptr;
    std::uint32_t hash = 0;
    std::uint32_t codeSize = 0;
    std::uint32_t argCount = 0;
    std::uint32_t localCount = 0;
    std::uint32_t globalCount = 0;
    std::uint32_t nativeCount = 0;
    void* localData = nullptr;
    void** globalData = nullptr;
    void** nativeEntrypoints = nullptr;
    std::uint32_t procCount = 0;
    std::uint32_t pad4C = 0;
    const char** procNames = nullptr;
    std::uint32_t nameHash = 0;
    std::uint32_t refCount = 0;
    const char* name = nullptr;
    const char** stringsData = nullptr;
    std::uint32_t stringsCount = 0;
    std::byte pad74[0x0C]{};
};

static_assert(sizeof(GTA_Script_Program_View) == 0x80);
static_assert(offsetof(GTA_Script_Program_View, codeBlocks) == 0x10);
static_assert(offsetof(GTA_Script_Program_View, nameHash) == 0x58);

using ScriptVm = int (*)(std::uint64_t* stack, std::int64_t** scriptGlobals,
                         GTA_Script_Program_View* program, void* context);

struct ScriptFunctionRuntime
{
    std::uintptr_t scriptGlobalsAddress = 0;
    std::uintptr_t programTableAddress = 0;
    std::uintptr_t scriptThreadsStorageAddress = 0;
    std::uintptr_t scriptVmAddress = 0;
    std::uint64_t buildFingerprint = 0;
    Backend::LoggerService* logger = nullptr;

    [[nodiscard]] bool Ready() const noexcept
    {
        return scriptGlobalsAddress != 0 && programTableAddress != 0 &&
               scriptThreadsStorageAddress != 0 && scriptVmAddress != 0 &&
               buildFingerprint != 0;
    }
};

struct ScriptFunctionRequest
{
    std::uint64_t requestId = 0;
    GTA_Script_Function_Descriptor descriptor;
    std::vector<GTA_Script_Value> arguments;
};

struct ScriptFunctionCacheEntry
{
    std::uint64_t descriptorKey = 0;
    std::uint64_t buildFingerprint = 0;
    std::uint32_t scriptHash = 0;
    std::uintptr_t programAddress = 0;
    std::uintptr_t codeBlocksAddress = 0;
    std::uint32_t codeSize = 0;
    std::uint32_t signatureProgramCounter = 0;
    std::uint32_t resolvedProgramCounter = 0;
};

enum class PatternScanState : std::uint8_t
{
    NotFound,
    Unique,
    Multiple,
    Unreadable
};

struct PatternScanResult
{
    PatternScanState state = PatternScanState::NotFound;
    std::uint32_t programCounter = 0;
};

struct ResolvedFunction
{
    std::uint32_t signatureProgramCounter = 0;
    std::uint32_t resolvedProgramCounter = 0;
    bool cacheHit = false;
};

std::mutex g_stateMutex;
std::mutex g_cacheMutex;
ScriptFunctionRuntime g_runtime{};
std::unordered_map<std::string, GTA_Script_Function_Descriptor> g_registry;
std::deque<ScriptFunctionRequest> g_queue;
std::unordered_map<std::uint64_t, GTA_Script_Function_Invoke_Snapshot> g_history;
std::deque<std::uint64_t> g_historyOrder;
GTA_Script_Function_Invoke_Snapshot g_latestSnapshot{};
std::uint64_t g_latestRequestId = 0;
std::uint64_t g_nextRequestId = 1;
std::unordered_map<std::string, ScriptFunctionCacheEntry> g_pcCache;
std::atomic_size_t g_pendingCount{0};

std::atomic_uint64_t g_requestsQueued{0};
std::atomic_uint64_t g_requestsRejected{0};
std::atomic_uint64_t g_invocationsExecuted{0};
std::atomic_uint64_t g_invocationsFailed{0};
std::atomic_uint64_t g_cacheHits{0};
std::atomic_uint64_t g_cacheMisses{0};
std::atomic_uint64_t g_signatureRevalidations{0};
std::atomic_uint64_t g_patternScans{0};

void ResetMetrics() noexcept
{
    g_requestsQueued.store(0, std::memory_order_relaxed);
    g_requestsRejected.store(0, std::memory_order_relaxed);
    g_invocationsExecuted.store(0, std::memory_order_relaxed);
    g_invocationsFailed.store(0, std::memory_order_relaxed);
    g_cacheHits.store(0, std::memory_order_relaxed);
    g_cacheMisses.store(0, std::memory_order_relaxed);
    g_signatureRevalidations.store(0, std::memory_order_relaxed);
    g_patternScans.store(0, std::memory_order_relaxed);
}

bool IsReadableAddress(std::uintptr_t address, std::size_t size) noexcept
{
    if (address == 0 || size == 0)
        return false;

    MEMORY_BASIC_INFORMATION memory{};
    if (::VirtualQuery(reinterpret_cast<const void*>(address), &memory, sizeof(memory)) == 0)
        return false;
    if (memory.State != MEM_COMMIT || (memory.Protect & (PAGE_GUARD | PAGE_NOACCESS)) != 0)
        return false;

    const DWORD protection = memory.Protect & 0xFFU;
    const bool readable = protection == PAGE_READONLY ||
                          protection == PAGE_READWRITE ||
                          protection == PAGE_WRITECOPY ||
                          protection == PAGE_EXECUTE_READ ||
                          protection == PAGE_EXECUTE_READWRITE ||
                          protection == PAGE_EXECUTE_WRITECOPY;
    if (!readable)
        return false;

    const auto start = reinterpret_cast<std::uintptr_t>(memory.BaseAddress);
    if (start > (std::numeric_limits<std::uintptr_t>::max)() - memory.RegionSize)
        return false;
    const auto end = start + memory.RegionSize;
    return address >= start && address <= end && size <= end - address;
}

bool IsWritableAddress(std::uintptr_t address, std::size_t size) noexcept
{
    if (!IsReadableAddress(address, size))
        return false;

    MEMORY_BASIC_INFORMATION memory{};
    if (::VirtualQuery(reinterpret_cast<const void*>(address), &memory, sizeof(memory)) == 0)
        return false;

    const DWORD protection = memory.Protect & 0xFFU;
    return protection == PAGE_READWRITE || protection == PAGE_WRITECOPY ||
           protection == PAGE_EXECUTE_READWRITE || protection == PAGE_EXECUTE_WRITECOPY;
}

std::uint32_t Joaat(std::string_view value) noexcept
{
