#include "GTA_Network_Session_Extension.hpp"

#include "GTA_Network_Session_State.hpp"
#include "Backend/Logging/LoggerService.hpp"

#include <Windows.h>
#include <intrin.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <string_view>

namespace Devilz::Integrations::GTA5_Enhanced
{
namespace
{
constexpr std::uint32_t JoinTypeGlobalIndex = 1575048U;
constexpr std::size_t ScriptProgramCount = 176;
constexpr std::size_t ScriptCodePageSize = 0x4000;
constexpr std::array<int, 11> SendToCloudsPattern{
    0x2D, 0x00, 0x02, 0x00, 0x00, 0x72, 0x5D, -1, -1, -1, 0x72
};

struct NetworkSessionRuntime
{
    std::uintptr_t scriptGlobalsAddress = 0;
    std::uintptr_t programTableAddress = 0;
    std::uintptr_t scriptThreadsStorageAddress = 0;
    std::uintptr_t scriptVmAddress = 0;
    Backend::LoggerService* logger = nullptr;
    std::uint32_t sendToCloudsPc = 0;
};

NetworkSessionRuntime g_runtime{};

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

GTA_Script_Thread_View* FindScriptThread(std::uint32_t scriptHash) noexcept
{
    if (!IsReadableAddress(g_runtime.scriptThreadsStorageAddress, sizeof(GTA_Script_Thread_Array_View)))
        return nullptr;

    GTA_Script_Thread_Array_View threads{};
    std::memcpy(&threads,
                reinterpret_cast<const void*>(g_runtime.scriptThreadsStorageAddress),
                sizeof(threads));

    const auto count = std::min<std::size_t>(threads.size, 256);
    if (!threads.data || count == 0 ||
        !IsReadableAddress(reinterpret_cast<std::uintptr_t>(threads.data), count * sizeof(void*))) {
        return nullptr;
    }

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

GTA_Script_Program_View* FindScriptProgram(std::uint32_t scriptHash) noexcept
{
    if (!IsReadableAddress(g_runtime.programTableAddress, ScriptProgramCount * sizeof(void*)))
        return nullptr;

    auto** programs = reinterpret_cast<GTA_Script_Program_View**>(g_runtime.programTableAddress);
    for (std::size_t index = 0; index < ScriptProgramCount; ++index) {
        GTA_Script_Program_View* program = nullptr;
        std::memcpy(&program, programs + index, sizeof(program));
        if (!program || !IsReadableAddress(reinterpret_cast<std::uintptr_t>(program), sizeof(GTA_Script_Program_View)))
            continue;
        if (program->nameHash == scriptHash && program->codeBlocks && program->codeSize != 0)
            return program;
    }

    return nullptr;
}

std::uint32_t FindSendToCloudsPc(GTA_Script_Program_View* program) noexcept
{
    if (!program || !program->codeBlocks || program->codeSize < SendToCloudsPattern.size())
        return 0;

    const std::size_t pageCount = (program->codeSize + ScriptCodePageSize - 1) / ScriptCodePageSize;
    if (!IsReadableAddress(reinterpret_cast<std::uintptr_t>(program->codeBlocks),
                           pageCount * sizeof(std::uint8_t*))) {
        return 0;
    }

    for (std::size_t pageIndex = 0; pageIndex < pageCount; ++pageIndex) {
        std::uint8_t* page = nullptr;
        std::memcpy(&page, program->codeBlocks + pageIndex, sizeof(page));
        if (!page)
            continue;

        const std::size_t pageStart = pageIndex * ScriptCodePageSize;
        const std::size_t remaining = program->codeSize - pageStart;
        const std::size_t pageSize = std::min<std::size_t>(remaining, ScriptCodePageSize);
        if (pageSize < SendToCloudsPattern.size() ||
            !IsReadableAddress(reinterpret_cast<std::uintptr_t>(page), pageSize)) {
            continue;
        }

        for (std::size_t offset = 0; offset + SendToCloudsPattern.size() <= pageSize; ++offset) {
            bool match = true;
            for (std::size_t byte = 0; byte < SendToCloudsPattern.size(); ++byte) {
                if (SendToCloudsPattern[byte] >= 0 &&
                    page[offset + byte] != static_cast<std::uint8_t>(SendToCloudsPattern[byte])) {
                    match = false;
                    break;
                }
            }
            if (match)
                return static_cast<std::uint32_t>(pageStart + offset);
        }
    }

    return 0;
}

GTA_Tls_Context_View* CurrentTls() noexcept
{
    const auto tlsArray = static_cast<std::uintptr_t>(__readgsqword(0x58));
    if (!IsReadableAddress(tlsArray, sizeof(void*)))
        return nullptr;

    GTA_Tls_Context_View* tls = nullptr;
    std::memcpy(&tls, reinterpret_cast<const void*>(tlsArray), sizeof(tls));
    if (!tls || !IsReadableAddress(reinterpret_cast<std::uintptr_t>(tls), sizeof(GTA_Tls_Context_View)))
        return nullptr;
    return tls;
}

bool CallSendToClouds() noexcept
{
    const auto shopControllerHash = Joaat("shop_controller");
    auto* thread = FindScriptThread(shopControllerHash);
    auto* program = FindScriptProgram(shopControllerHash);
    if (!thread || !program || !thread->stack)
        return false;

    if (g_runtime.sendToCloudsPc == 0)
        g_runtime.sendToCloudsPc = FindSendToCloudsPc(program);
    if (g_runtime.sendToCloudsPc == 0)
        return false;

    auto context = thread->context;
    const auto stackPointer = context.stackPointer;
    const auto stackAddress = reinterpret_cast<std::uintptr_t>(thread->stack + stackPointer);
    if (!IsWritableAddress(stackAddress, sizeof(std::uint64_t)))
        return false;

    auto* tls = CurrentTls();
    if (!tls)
        return false;

    const auto scriptVm = reinterpret_cast<ScriptVm>(g_runtime.scriptVmAddress);
    if (!scriptVm || !IsReadableAddress(g_runtime.scriptVmAddress, 1))
        return false;

    void* previousThread = tls->currentScriptThread;
    const bool previousActive = tls->scriptThreadActive;

    thread->stack[context.stackPointer++] = 0;
    context.programCounter = g_runtime.sendToCloudsPc;
    context.state = 0;

    tls->currentScriptThread = thread;
    tls->scriptThreadActive = true;
    (void)scriptVm(thread->stack,
                   reinterpret_cast<std::int64_t**>(g_runtime.scriptGlobalsAddress),
                   program,
                   &context);
    tls->scriptThreadActive = previousActive;
    tls->currentScriptThread = previousThread;

    return true;
}

int* ResolveJoinTypeGlobal() noexcept
{
    constexpr std::uint32_t blockIndex = (JoinTypeGlobalIndex >> 0x12U) & 0x3FU;
    constexpr std::uint32_t slotIndex = JoinTypeGlobalIndex & 0x3FFFFU;

    if (!IsReadableAddress(g_runtime.scriptGlobalsAddress, 64 * sizeof(std::int64_t*)))
        return nullptr;

    auto** globals = reinterpret_cast<std::int64_t**>(g_runtime.scriptGlobalsAddress);
    std::int64_t* block = nullptr;
    std::memcpy(&block, globals + blockIndex, sizeof(block));
    if (!block)
        return nullptr;

    auto* destination = reinterpret_cast<int*>(block + slotIndex);
    if (!IsWritableAddress(reinterpret_cast<std::uintptr_t>(destination), sizeof(int)))
        return nullptr;
    return destination;
}

bool LaunchJoinType(GTA_Network_Join_Type type) noexcept
{
    if (!CallSendToClouds())
        return false;

    auto* joinType = ResolveJoinTypeGlobal();
    if (!joinType)
        return false;

    *joinType = static_cast<int>(type);
    return true;
}
}

void ConfigureNetworkSessionExtension(
    std::uintptr_t scriptGlobalsAddress,
    std::uintptr_t programTableAddress,
    std::uintptr_t scriptThreadsStorageAddress,
    std::uintptr_t scriptVmAddress,
    Backend::LoggerService* logger) noexcept
{
    g_runtime = {};
    g_runtime.scriptGlobalsAddress = scriptGlobalsAddress;
    g_runtime.programTableAddress = programTableAddress;
    g_runtime.scriptThreadsStorageAddress = scriptThreadsStorageAddress;
    g_runtime.scriptVmAddress = scriptVmAddress;
    g_runtime.logger = logger;

    const bool ready = scriptGlobalsAddress != 0 && programTableAddress != 0 &&
                       scriptThreadsStorageAddress != 0 && scriptVmAddress != 0;
    GTA_Network_Session_State::Instance().SetRuntimeReady(ready);

    if (logger) {
        logger->Log(
            ready ? Backend::LogLevel::Info : Backend::LogLevel::Warning,
            ready
                ? "Network session switcher ready | shop_controller + ScriptVM path enabled"
                : "Network session switcher unavailable: ScriptGlobals, ProgramTable, ScriptThreads, or ScriptVM target missing",
            "GTA5_Enhanced.Network");
    }
}

void ResetNetworkSessionExtension() noexcept
{
    GTA_Network_Session_State::Instance().SetRuntimeReady(false);
    g_runtime = {};
}

void TickNetworkSessionExtension() noexcept
{
    auto request = GTA_Network_Session_State::Instance().TakePending();
    if (!request)
        return;

    const bool success = LaunchJoinType(*request);
    GTA_Network_Session_State::Instance().Complete(success);

    if (g_runtime.logger) {
        g_runtime.logger->Log(
            success ? Backend::LogLevel::Info : Backend::LogLevel::Warning,
            success
                ? "Session transition requested through shop_controller"
                : "Session transition request failed before GTA transition launch",
            "GTA5_Enhanced.Network");
    }
}
}
