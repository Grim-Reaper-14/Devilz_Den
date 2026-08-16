#include "GTA_Script_Function_Invoker.hpp"

#include <Windows.h>
#include <intrin.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cstring>
#include <limits>
#include <mutex>
#include <optional>
#include <sstream>
#include <string_view>
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
constexpr std::size_t MaxSavedStackTailSlots = 4096;

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
};

std::mutex g_mutex;
ScriptFunctionRuntime g_runtime{};
std::optional<ScriptFunctionRequest> g_pending;
GTA_Script_Function_Invoke_Snapshot g_snapshot{};
std::atomic_bool g_pendingFlag{false};
std::uint64_t g_nextRequestId = 1;

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
    std::uint32_t hash = 0;
    for (unsigned char character : value) {
        character = static_cast<unsigned char>(std::tolower(character));
        hash += character;
        hash += hash << 10U;
        hash ^= hash >> 6U;
    }
    hash += hash << 3U;
    hash ^= hash >> 11U;
    hash += hash << 15U;
    return hash;
}

bool DescriptorValid(const GTA_Script_Function_Descriptor& descriptor) noexcept
{
    if (descriptor.label.empty() || descriptor.scriptName.empty() ||
        descriptor.sourceReference.empty() || !descriptor.verifiedSynchronous ||
        descriptor.expectedBuildFingerprint == 0)
        return false;

    if (descriptor.bytePattern.size() < MinPatternBytes ||
        descriptor.bytePattern.size() > MaxPatternBytes)
        return false;

    std::size_t exactCount = 0;
    for (const auto value : descriptor.bytePattern) {
        if (value < -1 || value > 0xFF)
            return false;
        if (value >= 0)
            ++exactCount;
    }
    return exactCount >= MinExactPatternBytes;
}

GTA_Script_Thread_View* FindScriptThread(const ScriptFunctionRuntime& runtime,
                                         std::uint32_t scriptHash) noexcept
{
    if (!IsReadableAddress(runtime.scriptThreadsStorageAddress,
                           sizeof(GTA_Script_Thread_Array_View)))
        return nullptr;

    GTA_Script_Thread_Array_View threads{};
    std::memcpy(&threads,
                reinterpret_cast<const void*>(runtime.scriptThreadsStorageAddress),
                sizeof(threads));

    const auto count = std::min<std::size_t>(threads.size, MaxScriptThreads);
    if (!threads.data || count == 0 ||
        !IsReadableAddress(reinterpret_cast<std::uintptr_t>(threads.data),
                           count * sizeof(void*)))
        return nullptr;

    for (std::size_t index = 0; index < count; ++index) {
        GTA_Script_Thread_View* thread = nullptr;
        std::memcpy(&thread, threads.data + index, sizeof(thread));
        if (!thread || !IsReadableAddress(reinterpret_cast<std::uintptr_t>(thread), 0x154))
            continue;
        if (thread->context.threadId != 0 && thread->scriptHash == scriptHash)
            return thread;
    }
    return nullptr;
}

GTA_Script_Program_View* FindScriptProgram(const ScriptFunctionRuntime& runtime,
                                           std::uint32_t scriptHash) noexcept
{
    if (!IsReadableAddress(runtime.programTableAddress,
                           ScriptProgramCount * sizeof(void*)))
        return nullptr;

    auto** programs = reinterpret_cast<GTA_Script_Program_View**>(runtime.programTableAddress);
    for (std::size_t index = 0; index < ScriptProgramCount; ++index) {
        GTA_Script_Program_View* program = nullptr;
        std::memcpy(&program, programs + index, sizeof(program));
        if (!program ||
            !IsReadableAddress(reinterpret_cast<std::uintptr_t>(program),
                               sizeof(GTA_Script_Program_View)))
            continue;
        if (program->nameHash == scriptHash && program->codeBlocks && program->codeSize != 0)
            return program;
    }
    return nullptr;
}

bool ResolveCodeByte(GTA_Script_Program_View* program,
                     std::uint32_t programCounter,
                     std::uint8_t& out) noexcept
{
    if (!program || !program->codeBlocks || programCounter >= program->codeSize)
        return false;

    const std::size_t pageIndex = programCounter / ScriptCodePageSize;
    const std::size_t pageOffset = programCounter % ScriptCodePageSize;
    const std::size_t pageCount =
        (static_cast<std::size_t>(program->codeSize) + ScriptCodePageSize - 1) /
        ScriptCodePageSize;
    if (pageIndex >= pageCount ||
        !IsReadableAddress(reinterpret_cast<std::uintptr_t>(program->codeBlocks),
                           pageCount * sizeof(std::uint8_t*)))
        return false;

    std::uint8_t* page = nullptr;
    std::memcpy(&page, program->codeBlocks + pageIndex, sizeof(page));
    if (!page ||
        !IsReadableAddress(reinterpret_cast<std::uintptr_t>(page + pageOffset), 1))
        return false;

    out = page[pageOffset];
    return true;
}

bool PatternMatchesAt(GTA_Script_Program_View* program,
                      std::uint32_t programCounter,
                      const std::vector<std::int16_t>& pattern) noexcept
{
    if (!program || pattern.empty() || programCounter >= program->codeSize ||
        pattern.size() > static_cast<std::size_t>(program->codeSize - programCounter))
        return false;

    for (std::size_t index = 0; index < pattern.size(); ++index) {
        if (pattern[index] < 0)
            continue;
        std::uint8_t value = 0;
        if (!ResolveCodeByte(program,
                             programCounter + static_cast<std::uint32_t>(index),
                             value) ||
            value != static_cast<std::uint8_t>(pattern[index]))
            return false;
    }
    return true;
}

std::optional<std::uint32_t> FindUniquePattern(
    GTA_Script_Program_View* program,
    const std::vector<std::int16_t>& pattern) noexcept
{
    if (!program || !program->codeBlocks || pattern.empty() || pattern.size() > program->codeSize)
        return std::nullopt;

    const std::size_t pageCount =
        (static_cast<std::size_t>(program->codeSize) + ScriptCodePageSize - 1) /
        ScriptCodePageSize;
    if (!IsReadableAddress(reinterpret_cast<std::uintptr_t>(program->codeBlocks),
                           pageCount * sizeof(std::uint8_t*)))
        return std::nullopt;

    std::optional<std::uint32_t> match;
    auto rememberMatch = [&](std::uint32_t pc) noexcept -> bool {
        if (match)
            return false;
        match = pc;
        return true;
    };

    for (std::size_t pageIndex = 0; pageIndex < pageCount; ++pageIndex) {
        std::uint8_t* page = nullptr;
        std::memcpy(&page, program->codeBlocks + pageIndex, sizeof(page));
        if (!page)
            return std::nullopt;

        const std::size_t pageStart = pageIndex * ScriptCodePageSize;
        const std::size_t remaining = program->codeSize - pageStart;
        const std::size_t pageSize = std::min<std::size_t>(remaining, ScriptCodePageSize);
        if (!IsReadableAddress(reinterpret_cast<std::uintptr_t>(page), pageSize))
            return std::nullopt;

        // Fast path: almost every candidate is wholly inside one 0x4000 byte
        // code page. Validate the page once, then compare bytes directly.
        if (pageSize >= pattern.size()) {
            const std::size_t lastLocal = pageSize - pattern.size();
            for (std::size_t local = 0; local <= lastLocal; ++local) {
                bool equal = true;
                for (std::size_t byte = 0; byte < pattern.size(); ++byte) {
                    if (pattern[byte] >= 0 &&
                        page[local + byte] != static_cast<std::uint8_t>(pattern[byte])) {
                        equal = false;
                        break;
                    }
                }
                if (equal && !rememberMatch(static_cast<std::uint32_t>(pageStart + local)))
                    return std::nullopt;
            }
        }

        // Only a tiny boundary window can span two pages. Use the slower
        // page-aware verifier just for those candidates.
        if (pageIndex + 1 < pageCount && pattern.size() > 1) {
            const std::size_t firstBoundary =
                pageSize > pattern.size() - 1 ? pageSize - (pattern.size() - 1) : 0;
            for (std::size_t local = firstBoundary; local < pageSize; ++local) {
                const auto pc = static_cast<std::uint32_t>(pageStart + local);
                if (pattern.size() > static_cast<std::size_t>(program->codeSize - pc))
                    continue;
                if (PatternMatchesAt(program, pc, pattern) && !rememberMatch(pc))
                    return std::nullopt;
            }
        }
    }
    return match;
}

GTA_Tls_Context_View* CurrentTls() noexcept
{
    const auto tlsArray = static_cast<std::uintptr_t>(__readgsqword(0x58));
    if (!IsReadableAddress(tlsArray, sizeof(void*)))
        return nullptr;

    GTA_Tls_Context_View* tls = nullptr;
    std::memcpy(&tls, reinterpret_cast<const void*>(tlsArray), sizeof(tls));
    if (!tls || !IsReadableAddress(reinterpret_cast<std::uintptr_t>(tls),
                                   sizeof(GTA_Tls_Context_View)))
        return nullptr;
    return tls;
}

std::string HexPc(std::uint32_t pc)
{
    std::ostringstream stream;
    stream << "0x" << std::hex << std::uppercase << pc;
    return stream.str();
}

GTA_Script_Function_Invoke_Snapshot Execute(
    const ScriptFunctionRuntime& runtime,
    const ScriptFunctionRequest& request) noexcept
{
    GTA_Script_Function_Invoke_Snapshot result{};
    result.requestId = request.requestId;
    result.status = GTA_Script_Function_Invoke_Status::Failed;
    result.label = request.descriptor.label;
    result.scriptName = request.descriptor.scriptName;

    const auto& descriptor = request.descriptor;
    if (!runtime.Ready()) {
        result.status = GTA_Script_Function_Invoke_Status::RuntimeUnavailable;
        result.detail = "ScriptGlobals, ProgramTable, ScriptThreads, or ScriptVM is unavailable";
        return result;
    }
    if (runtime.buildFingerprint != descriptor.expectedBuildFingerprint) {
        result.status = GTA_Script_Function_Invoke_Status::UnsupportedBuild;
        result.detail = "Descriptor build fingerprint does not match the running GTA build";
        return result;
    }
    if (!DescriptorValid(descriptor)) {
        result.status = GTA_Script_Function_Invoke_Status::InvalidDescriptor;
        result.detail = "Descriptor is missing verified synchronous/source identity data or a sufficiently strong byte signature";
        return result;
    }

    const auto scriptHash = Joaat(descriptor.scriptName);
    auto* program = FindScriptProgram(runtime, scriptHash);
    if (!program) {
        result.status = GTA_Script_Function_Invoke_Status::ScriptProgramNotFound;
        result.detail = "Target Rockstar script program is not loaded";
        return result;
    }

    std::uint32_t programCounter = descriptor.programCounter;
    if (descriptor.locator == GTA_Script_Function_Locator_Kind::UniqueBytePattern) {
        const auto patternPc = FindUniquePattern(program, descriptor.bytePattern);
        if (!patternPc) {
            result.status = GTA_Script_Function_Invoke_Status::PatternNotUnique;
            result.detail = "Byte pattern was not found exactly once in the loaded script";
            return result;
        }

        const auto adjusted = static_cast<std::int64_t>(*patternPc) +
                              static_cast<std::int64_t>(descriptor.entryAdjustment);
        if (adjusted < 0 || adjusted >= program->codeSize) {
            result.status = GTA_Script_Function_Invoke_Status::ProgramCounterOutOfRange;
            result.detail = "Pattern entry adjustment resolves outside the script bytecode";
            return result;
        }
        programCounter = static_cast<std::uint32_t>(adjusted);
    } else {
        if (programCounter >= program->codeSize) {
            result.status = GTA_Script_Function_Invoke_Status::ProgramCounterOutOfRange;
            result.detail = "Fixed program counter lies outside the loaded script bytecode";
            return result;
        }
        if (!PatternMatchesAt(program, programCounter, descriptor.bytePattern)) {
            result.status = GTA_Script_Function_Invoke_Status::SignatureMismatch;
            result.detail = "Loaded script bytes do not match the descriptor signature";
            return result;
        }
    }
    result.resolvedProgramCounter = programCounter;

    auto* thread = FindScriptThread(runtime, scriptHash);
    if (!thread) {
        result.status = GTA_Script_Function_Invoke_Status::ScriptThreadNotFound;
        result.detail = "No live thread exists for the target Rockstar script";
        return result;
    }

    if (!thread->stack || thread->context.stackSize == 0 ||
        thread->context.stackSize > MaxScriptStackSlots ||
        thread->context.stackPointer >= thread->context.stackSize) {
        result.status = GTA_Script_Function_Invoke_Status::StackUnavailable;
        result.detail = "Target script thread stack metadata failed validation";
        return result;
    }

    const std::size_t stackPointer = thread->context.stackPointer;
    const std::size_t tailSlots = thread->context.stackSize - stackPointer;
    if (tailSlots == 0 || tailSlots > MaxSavedStackTailSlots ||
        !IsWritableAddress(reinterpret_cast<std::uintptr_t>(thread->stack + stackPointer),
                           tailSlots * sizeof(std::uint64_t))) {
        result.status = GTA_Script_Function_Invoke_Status::StackUnavailable;
        result.detail = "Target script thread stack tail is not safely preservable";
        return result;
    }

    auto* tls = CurrentTls();
    const auto scriptVm = reinterpret_cast<ScriptVm>(runtime.scriptVmAddress);
    if (!tls || !scriptVm || !IsReadableAddress(runtime.scriptVmAddress, 1)) {
        result.status = GTA_Script_Function_Invoke_Status::ScriptVmUnavailable;
        result.detail = "ScriptVM or GTA TLS script context is unavailable";
        return result;
    }

    // The proven shop_controller path uses the live thread stack while passing
    // a private context copy into ScriptVM. Preserve every slot the temporary
    // call can touch so the original Rockstar thread resumes with an unchanged
    // stack after our one-shot function returns.
    thread_local std::array<std::uint64_t, MaxSavedStackTailSlots> savedStackTail{};
    std::memcpy(savedStackTail.data(), thread->stack + stackPointer,
                tailSlots * sizeof(std::uint64_t));

    auto context = thread->context;
    thread->stack[context.stackPointer++] = 0; // synthetic return PC
    context.programCounter = programCounter;
    context.state = 0;

    void* previousThread = tls->currentScriptThread;
    const bool previousActive = tls->scriptThreadActive;
    tls->currentScriptThread = thread;
    tls->scriptThreadActive = true;

    result.scriptVmResult = scriptVm(
        thread->stack,
        reinterpret_cast<std::int64_t**>(runtime.scriptGlobalsAddress),
        program,
        &context);

    tls->scriptThreadActive = previousActive;
    tls->currentScriptThread = previousThread;

    std::memcpy(thread->stack + stackPointer, savedStackTail.data(),
                tailSlots * sizeof(std::uint64_t));

    result.status = GTA_Script_Function_Invoke_Status::Executed;
    result.detail = "Verified zero-argument Rockstar script function executed at PC " +
                    HexPc(programCounter) + "; temporary stack changes restored";
    return result;
}

void LogCompletion(const ScriptFunctionRuntime& runtime,
                   const GTA_Script_Function_Invoke_Snapshot& result) noexcept
{
    if (!runtime.logger)
        return;

    const bool ok = result.status == GTA_Script_Function_Invoke_Status::Executed;
    runtime.logger->Log(
        ok ? Backend::LogLevel::Info : Backend::LogLevel::Warning,
        result.label + " | " + GTA_Script_Function_Invoke_Status_Name(result.status) +
            " | " + result.detail,
        "GTA5_Enhanced.ScriptFunction");
}
}

const char* GTA_Script_Function_Invoke_Status_Name(
    GTA_Script_Function_Invoke_Status status) noexcept
{
    switch (status) {
    case GTA_Script_Function_Invoke_Status::Idle: return "IDLE";
    case GTA_Script_Function_Invoke_Status::Queued: return "QUEUED";
    case GTA_Script_Function_Invoke_Status::Running: return "RUNNING";
    case GTA_Script_Function_Invoke_Status::Executed: return "EXECUTED";
    case GTA_Script_Function_Invoke_Status::RuntimeUnavailable: return "RUNTIME UNAVAILABLE";
    case GTA_Script_Function_Invoke_Status::UnsupportedBuild: return "UNSUPPORTED BUILD";
    case GTA_Script_Function_Invoke_Status::InvalidDescriptor: return "INVALID DESCRIPTOR";
    case GTA_Script_Function_Invoke_Status::ScriptProgramNotFound: return "SCRIPT PROGRAM NOT FOUND";
    case GTA_Script_Function_Invoke_Status::ScriptThreadNotFound: return "SCRIPT THREAD NOT FOUND";
    case GTA_Script_Function_Invoke_Status::ProgramCounterOutOfRange: return "PROGRAM COUNTER OUT OF RANGE";
    case GTA_Script_Function_Invoke_Status::SignatureMismatch: return "SIGNATURE MISMATCH";
    case GTA_Script_Function_Invoke_Status::PatternNotUnique: return "PATTERN NOT UNIQUE";
    case GTA_Script_Function_Invoke_Status::StackUnavailable: return "STACK UNAVAILABLE";
    case GTA_Script_Function_Invoke_Status::ScriptVmUnavailable: return "SCRIPT VM UNAVAILABLE";
    default: return "FAILED";
    }
}

void ConfigureScriptFunctionInvoker(
    std::uintptr_t scriptGlobalsAddress,
    std::uintptr_t programTableAddress,
    std::uintptr_t scriptThreadsStorageAddress,
    std::uintptr_t scriptVmAddress,
    std::uint64_t buildFingerprint,
    Backend::LoggerService* logger) noexcept
{
    std::scoped_lock lock(g_mutex);
    g_runtime = {
        scriptGlobalsAddress,
        programTableAddress,
        scriptThreadsStorageAddress,
        scriptVmAddress,
        buildFingerprint,
        logger,
    };
    g_pending.reset();
    g_pendingFlag.store(false, std::memory_order_release);
    g_snapshot = {};
    g_nextRequestId = std::max<std::uint64_t>(g_nextRequestId, 1);

    if (logger) {
        logger->Log(
            g_runtime.Ready() ? Backend::LogLevel::Info : Backend::LogLevel::Warning,
            g_runtime.Ready()
                ? "Verified Rockstar ScriptVM function invoker ready | zero-argument registered calls only"
                : "Script function invoker unavailable: ScriptGlobals, ProgramTable, ScriptThreads, or ScriptVM target missing",
            "GTA5_Enhanced.ScriptFunction");
    }
}

void ResetScriptFunctionInvoker() noexcept
{
    std::scoped_lock lock(g_mutex);
    g_runtime = {};
    g_pending.reset();
    g_pendingFlag.store(false, std::memory_order_release);
    g_snapshot = {};
    // Request ids remain monotonic across runtime resets so stale menu ids
    // cannot alias a future invocation after GTA/runtime restart.
    g_nextRequestId = std::max<std::uint64_t>(g_nextRequestId, 1);
}

std::uint64_t RequestVerifiedZeroArgumentScriptFunction(
    GTA_Script_Function_Descriptor descriptor) noexcept
{
    std::scoped_lock lock(g_mutex);

    if (g_pending)
        return 0;

    const auto requestId = g_nextRequestId++;
    if (g_nextRequestId == 0)
        g_nextRequestId = 1;

    g_snapshot = {};
    g_snapshot.requestId = requestId;
    g_snapshot.label = descriptor.label;
    g_snapshot.scriptName = descriptor.scriptName;

    if (!g_runtime.Ready()) {
        g_snapshot.status = GTA_Script_Function_Invoke_Status::RuntimeUnavailable;
        g_snapshot.detail = "Script function runtime is not configured";
        return requestId;
    }
    if (!DescriptorValid(descriptor)) {
        g_snapshot.status = GTA_Script_Function_Invoke_Status::InvalidDescriptor;
        g_snapshot.detail = "Descriptor is missing verified synchronous/source identity data or a sufficiently strong byte signature";
        return requestId;
    }
    if (descriptor.expectedBuildFingerprint != g_runtime.buildFingerprint) {
        g_snapshot.status = GTA_Script_Function_Invoke_Status::UnsupportedBuild;
        g_snapshot.detail = "Descriptor does not target the running GTA build";
        return requestId;
    }
    g_pending = ScriptFunctionRequest{requestId, std::move(descriptor)};
    g_snapshot.status = GTA_Script_Function_Invoke_Status::Queued;
    g_snapshot.detail = "Queued for the existing RunScriptThreads game-thread bridge";
    g_pendingFlag.store(true, std::memory_order_release);
    return requestId;
}

GTA_Script_Function_Invoke_Snapshot ScriptFunctionInvokeSnapshot()
{
    std::scoped_lock lock(g_mutex);
    return g_snapshot;
}

bool HasPendingScriptFunctionInvocation() noexcept
{
    return g_pendingFlag.load(std::memory_order_acquire);
}

bool TickScriptFunctionInvoker() noexcept
{
    if (!g_pendingFlag.load(std::memory_order_acquire))
        return false;

    ScriptFunctionRuntime runtime{};
    std::optional<ScriptFunctionRequest> request;
    {
        std::scoped_lock lock(g_mutex);
        if (!g_pending) {
            g_pendingFlag.store(false, std::memory_order_release);
            return false;
        }
        runtime = g_runtime;
        request = std::move(g_pending);
        g_pending.reset();
        g_pendingFlag.store(false, std::memory_order_release);
        g_snapshot.status = GTA_Script_Function_Invoke_Status::Running;
        g_snapshot.detail = "Executing through Rockstar ScriptVM on the GTA game thread";
    }

    auto result = Execute(runtime, *request);
    LogCompletion(runtime, result);

    {
        std::scoped_lock lock(g_mutex);
        if (g_snapshot.requestId == result.requestId)
            g_snapshot = std::move(result);
    }
    return true;
}
}
