#include "GTA_Random_Events_Extension.hpp"

#include "Backend/Logging/LoggerService.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Manager.hpp"

#include <Windows.h>
#include <intrin.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace Devilz::Integrations::GTA5_Enhanced
{
namespace
{
constexpr std::uint64_t SupportedBuildFingerprint = 0x6A4F97F605B81000ULL;
constexpr std::uint32_t GsbdRandomEventsGlobal = 1882345U;
constexpr std::uint32_t GpbdFm2Global = 1882797U;
constexpr std::uint32_t FreemodeRandomEventsLocal = 16199U;
constexpr std::size_t GpbdFm2EntrySlots = 321;
constexpr std::size_t RandomEventsClientOffset = 82;
constexpr std::size_t ScriptProgramCount = 176;
constexpr std::size_t ScriptCodePageSize = 0x4000;
constexpr GTA_Native_Hash GetNetworkTimeHash = 0x7E3F74F641EE6B27ULL;
constexpr GTA_Native_Hash SendTuScriptEventHash = 0x71A6F836422FDD2BULL;
constexpr std::int32_t RequestRandomEventIndex = -126218586;

constexpr std::array<const char*, GTA_Random_Event_Count> EventNames{
    "Drug Vehicle",
    "Movie Props",
    "Sleeping Guard",
    "Exotic Exports",
    "The Slashers",
    "Phantom Car",
    "Sightseeing",
    "Smuggler Trail",
    "Cerberus Surprise",
    "Smuggler Plane",
    "Crime Scene",
    "Metal Detector",
    "Finders Keepers",
    "Shop Robbery",
    "The Gooch",
    "Weazel Plaza Shootout",
    "Armored Truck",
    "Possessed Animals",
    "Ghosts Exposed",
    "Happy Holidays Hauler",
    "Community Outreach",
    "Getaway Driver",
    "Stoner Survival",
    "Valentine Cheater"
};

constexpr std::array<const char*, GTA_Random_Event_Count> EventScriptNames{
    "fm_content_drug_vehicle",
    "fm_content_movie_props",
    "fm_content_golden_gun",
    "fm_content_vehicle_list",
    "fm_content_slasher",
    "fm_content_phantom_car",
    "fm_content_sightseeing",
    "fm_content_smuggler_trail",
    "fm_content_cerberus",
    "fm_content_smuggler_plane",
    "fm_content_crime_scene",
    "fm_content_metal_detector",
    "fm_content_convoy",
    "fm_content_robbery",
    "fm_content_xmas_mugger",
    "fm_content_bank_shootout",
    "fm_content_armoured_truck",
    "fm_content_possessed_animals",
    "fm_content_ghosthunt",
    "fm_content_xmas_truck",
    "fm_content_community_outreach",
    "fm_content_getaway_driver",
    "fm_content_survival_grouping",
    "fm_content_valentine_cheater"
};

constexpr std::array<int, 9> GetNumVariationsPattern{
    0x5D, -1, -1, -1, 0x01, 0x72, 0x02, 0x39, 0x04
};
constexpr std::array<int, 8> SetServerStatePattern{
    0x5D, -1, -1, -1, 0x55, 0x2E, 0x00, 0x5D
};
constexpr std::array<int, 8> SetClientStatePattern{
    0x5D, -1, -1, -1, 0x55, 0x08, 0x00, 0x74
};

template <typename T>
struct alignas(8) ScriptSlot
{
    T value{};
};

static_assert(sizeof(ScriptSlot<std::int32_t>) == 8);
static_assert(sizeof(ScriptSlot<float>) == 8);

struct ScriptTimer
{
    ScriptSlot<std::int32_t> time;
    ScriptSlot<std::int32_t> initialized;
};

struct ScriptVector3
{
    ScriptSlot<float> x;
    ScriptSlot<float> y;
    ScriptSlot<float> z;
};

struct RandomEventServerData
{
    ScriptSlot<std::int32_t> state;
    ScriptTimer timer;
    ScriptSlot<std::int32_t> flagsArraySize;
    ScriptSlot<std::int32_t> flags;
    ScriptSlot<std::int32_t> variation;
    ScriptSlot<std::int32_t> subvariation;
    ScriptSlot<std::int32_t> reservedPeds;
    ScriptSlot<std::int32_t> reservedVehicles;
    ScriptSlot<std::int32_t> reservedObjects;
    ScriptVector3 triggerPosition;
    ScriptSlot<float> triggerRange;
    ScriptSlot<std::int32_t> retries;
};

static_assert(sizeof(RandomEventServerData) == 15 * sizeof(std::uint64_t));
static_assert(offsetof(RandomEventServerData, state) == 0 * sizeof(std::uint64_t));
static_assert(offsetof(RandomEventServerData, subvariation) == 6 * sizeof(std::uint64_t));
static_assert(offsetof(RandomEventServerData, triggerPosition) == 10 * sizeof(std::uint64_t));
static_assert(offsetof(RandomEventServerData, triggerRange) == 13 * sizeof(std::uint64_t));

struct GsbdRandomEvents
{
    ScriptSlot<std::int32_t> initState;
    ScriptSlot<std::int32_t> eventArraySize;
    std::array<RandomEventServerData, GTA_Random_Event_Count> events;
    std::array<std::uint64_t, 5> tail;
};

static_assert(sizeof(GsbdRandomEvents) == 367 * sizeof(std::uint64_t));
static_assert(offsetof(GsbdRandomEvents, events) == 2 * sizeof(std::uint64_t));

struct RandomEventClientData
{
    ScriptSlot<std::int32_t> state;
    ScriptSlot<std::int32_t> flagsArraySize;
    ScriptSlot<std::int32_t> flags;
};

static_assert(sizeof(RandomEventClientData) == 3 * sizeof(std::uint64_t));

struct RandomEventsClientData
{
    ScriptSlot<std::int32_t> initState;
    ScriptSlot<std::int32_t> eventArraySize;
    std::array<RandomEventClientData, GTA_Random_Event_Count> events;
    ScriptSlot<std::int32_t> participantFlags;
    ScriptSlot<std::int32_t> unused;
};

static_assert(sizeof(RandomEventsClientData) == 76 * sizeof(std::uint64_t));
static_assert(offsetof(RandomEventsClientData, events) == 2 * sizeof(std::uint64_t));

struct FreemodeRandomEventData
{
    ScriptSlot<std::int32_t> registeredEvents;
    ScriptSlot<std::int32_t> triggerArraySize;
    ScriptSlot<std::int32_t> triggerPointer;
    ScriptSlot<std::int32_t> triggerPositionPointer;
    ScriptSlot<std::int32_t> triggerRangePointer;
    ScriptSlot<std::int32_t> availablePointer;
    ScriptSlot<std::int32_t> inactiveTime;
    ScriptSlot<std::int32_t> availableTime;
    ScriptSlot<std::int32_t> flags;
    ScriptTimer lastTriggerAttempt;
    ScriptSlot<std::int32_t> unused;
};

static_assert(sizeof(FreemodeRandomEventData) == 12 * sizeof(std::uint64_t));
static_assert(offsetof(FreemodeRandomEventData, inactiveTime) == 6 * sizeof(std::uint64_t));
static_assert(offsetof(FreemodeRandomEventData, availableTime) == 7 * sizeof(std::uint64_t));

struct FreemodeRandomEventsData
{
    ScriptSlot<std::int32_t> eventArraySize;
    std::array<FreemodeRandomEventData, GTA_Random_Event_Count> events;
    ScriptSlot<std::int32_t> missionEventCount;
    ScriptSlot<std::int32_t> fmmcArraySize;
    std::array<ScriptSlot<std::int32_t>, GTA_Random_Event_Count> fmmcTypes;
};

static_assert(sizeof(FreemodeRandomEventsData) == 315 * sizeof(std::uint64_t));
static_assert(offsetof(FreemodeRandomEventsData, events) == 1 * sizeof(std::uint64_t));
static_assert(offsetof(FreemodeRandomEventsData, fmmcTypes) == 291 * sizeof(std::uint64_t));

struct RequestRandomEventPayload
{
    ScriptSlot<std::int32_t> eventIndex;
    ScriptSlot<std::int32_t> senderIndex;
    ScriptSlot<std::int32_t> playerBits;
    ScriptSlot<std::int32_t> fmmcType;
    ScriptSlot<std::int32_t> variation;
    ScriptSlot<std::int32_t> subvariation;
    ScriptSlot<std::int32_t> playersToSend;
};

static_assert(sizeof(RequestRandomEventPayload) == 7 * sizeof(std::uint64_t));
static_assert(offsetof(RequestRandomEventPayload, fmmcType) == 3 * sizeof(std::uint64_t));
static_assert(offsetof(RequestRandomEventPayload, subvariation) == 5 * sizeof(std::uint64_t));
static_assert(offsetof(RequestRandomEventPayload, playersToSend) == 6 * sizeof(std::uint64_t));

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
    char scriptName[64]{};
    std::byte pad194[4]{};
    void* scriptHandler = nullptr;
    void* netComponent = nullptr;
};

static_assert(offsetof(GTA_Script_Thread_View, context) == 0x08);
static_assert(offsetof(GTA_Script_Thread_View, stack) == 0xB8);
static_assert(offsetof(GTA_Script_Thread_View, scriptHash) == 0x150);
static_assert(offsetof(GTA_Script_Thread_View, netComponent) == 0x1A0);

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

enum class RandomEventCommandType : std::uint8_t
{
    Launch,
    Kill,
    Teleport,
    SetCooldown,
    SetAvailability
};

struct RandomEventCommand
{
    RandomEventCommandType type = RandomEventCommandType::Launch;
    GTA_Random_Event_Id event = GTA_Random_Event_Id::DrugVehicle;
    int value = 0;
};

struct RandomEventActionResult
{
    GTA_Random_Event_Action_Status status = GTA_Random_Event_Action_Status::Failed;
    std::string detail;
};

struct RandomEventsRuntime
{
    std::uintptr_t scriptGlobalsAddress = 0;
    std::uintptr_t programTableAddress = 0;
    std::uintptr_t scriptThreadsStorageAddress = 0;
    std::uintptr_t scriptVmAddress = 0;
    std::uint64_t buildFingerprint = 0;
    Backend::LoggerService* logger = nullptr;
    bool ready = false;
    GTA_Script_Thread_View* freemodeThread = nullptr;
    std::uint32_t freemodeThreadId = 0;
    GTA_Script_Program_View* freemodeProgram = nullptr;
    std::uint32_t getNumVariationsPc = 0;
    bool getNumVariationsScanned = false;
    std::array<int, GTA_Random_Event_Count> maxSubvariations{};
    std::array<bool, GTA_Random_Event_Count> variationQueried{};
    std::array<GTA_Script_Program_View*, GTA_Random_Event_Count> killPrograms{};
    std::array<std::uint32_t, GTA_Random_Event_Count> setServerStatePcs{};
    std::array<std::uint32_t, GTA_Random_Event_Count> setClientStatePcs{};
};

struct ResolvedRandomEventsContext
{
    GsbdRandomEvents* server = nullptr;
    RandomEventsClientData* client = nullptr;
    FreemodeRandomEventsData* freemode = nullptr;
    GTA_Script_Thread_View* freemodeThread = nullptr;
    int player = -1;
};

RandomEventsRuntime g_runtime{};
std::mutex g_stateMutex;
GTA_Random_Events_Snapshot g_snapshot{};
std::optional<RandomEventCommand> g_pendingCommand;
bool g_commandInFlight = false;
std::atomic<std::size_t> g_selectedEvent{0};

bool ValidEvent(GTA_Random_Event_Id event) noexcept
{
    return static_cast<std::size_t>(event) < GTA_Random_Event_Count;
}

std::uint32_t Joaat(std::string_view value) noexcept
{
    std::uint32_t hash = 0;
    for (const char rawCharacter : value) {
        auto character = static_cast<unsigned char>(rawCharacter);
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

void* ResolveGlobal(std::uint32_t index, std::size_t size, bool writable) noexcept
{
    if (!IsReadableAddress(g_runtime.scriptGlobalsAddress, 64 * sizeof(std::int64_t*)))
        return nullptr;

    const std::uint32_t blockIndex = (index >> 0x12U) & 0x3FU;
    const std::uint32_t slotIndex = index & 0x3FFFFU;
    auto** globals = reinterpret_cast<std::int64_t**>(g_runtime.scriptGlobalsAddress);

    std::int64_t* block = nullptr;
    std::memcpy(&block, globals + blockIndex, sizeof(block));
    if (!block)
        return nullptr;

    auto* address = reinterpret_cast<void*>(block + slotIndex);
    const auto numericAddress = reinterpret_cast<std::uintptr_t>(address);
    if (writable ? !IsWritableAddress(numericAddress, size)
                 : !IsReadableAddress(numericAddress, size)) {
        return nullptr;
    }
    return address;
}

template <typename T>
T* ResolveGlobal(std::uint32_t index, bool writable = false) noexcept
{
    return static_cast<T*>(ResolveGlobal(index, sizeof(T), writable));
}

GTA_Script_Thread_View* FindScriptThread(std::uint32_t scriptHash) noexcept
{
    if (!IsReadableAddress(g_runtime.scriptThreadsStorageAddress, sizeof(GTA_Script_Thread_Array_View)))
        return nullptr;

    GTA_Script_Thread_Array_View threads{};
    std::memcpy(&threads,
                reinterpret_cast<const void*>(g_runtime.scriptThreadsStorageAddress),
                sizeof(threads));

    const auto count = (std::min<std::size_t>)(threads.size, 256);
    if (!threads.data || count == 0 ||
        !IsReadableAddress(reinterpret_cast<std::uintptr_t>(threads.data), count * sizeof(void*))) {
        return nullptr;
    }

    for (std::size_t index = 0; index < count; ++index) {
        GTA_Script_Thread_View* thread = nullptr;
        std::memcpy(&thread, threads.data + index, sizeof(thread));
        if (!thread ||
            !IsReadableAddress(reinterpret_cast<std::uintptr_t>(thread), sizeof(GTA_Script_Thread_View))) {
            continue;
        }
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
        if (!program ||
            !IsReadableAddress(reinterpret_cast<std::uintptr_t>(program), sizeof(GTA_Script_Program_View))) {
            continue;
        }
        if (program->nameHash == scriptHash && program->codeBlocks && program->codeSize != 0)
            return program;
    }

    return nullptr;
}

template <std::size_t Size>
std::uint32_t FindScriptFunctionPc(
    GTA_Script_Program_View* program,
    const std::array<int, Size>& pattern) noexcept
{
    if (!program || !program->codeBlocks || program->codeSize < pattern.size())
        return 0;

    const std::size_t pageCount =
        (program->codeSize + ScriptCodePageSize - 1) / ScriptCodePageSize;
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
        const std::size_t pageSize = (std::min)(remaining, ScriptCodePageSize);
        if (pageSize < pattern.size() ||
            !IsReadableAddress(reinterpret_cast<std::uintptr_t>(page), pageSize)) {
            continue;
        }

        for (std::size_t offset = 0; offset + pattern.size() <= pageSize; ++offset) {
            bool matches = true;
            for (std::size_t byte = 0; byte < pattern.size(); ++byte) {
                if (pattern[byte] >= 0 &&
                    page[offset + byte] != static_cast<std::uint8_t>(pattern[byte])) {
                    matches = false;
                    break;
                }
            }
            if (!matches)
                continue;

            if (offset + 4 > pageSize)
                return 0;
            return static_cast<std::uint32_t>(page[offset + 1]) |
                   (static_cast<std::uint32_t>(page[offset + 2]) << 8U) |
                   (static_cast<std::uint32_t>(page[offset + 3]) << 16U);
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
    if (!tls || !IsReadableAddress(reinterpret_cast<std::uintptr_t>(tls), sizeof(*tls)))
        return nullptr;
    return tls;
}

bool CallScriptFunction(
    GTA_Script_Thread_View* thread,
    GTA_Script_Program_View* program,
    std::uint32_t programCounter,
    const std::uint64_t* arguments,
    std::size_t argumentCount,
    std::uint64_t* result = nullptr) noexcept
{
    if (!thread || !program || !thread->stack || programCounter == 0 || argumentCount > 16)
        return false;

    auto context = thread->context;
    const std::size_t requiredSlots = argumentCount + 1;
    if (context.stackPointer > context.stackSize ||
        requiredSlots > static_cast<std::size_t>(context.stackSize - context.stackPointer)) {
        return false;
    }

    const auto top = context.stackPointer;
    const auto stackAddress = reinterpret_cast<std::uintptr_t>(thread->stack + top);
    if (!IsWritableAddress(stackAddress, requiredSlots * sizeof(std::uint64_t)))
        return false;

    auto* tls = CurrentTls();
    const auto scriptVm = reinterpret_cast<ScriptVm>(g_runtime.scriptVmAddress);
    if (!tls || !scriptVm || !IsReadableAddress(g_runtime.scriptVmAddress, 1))
        return false;

    std::array<std::uint64_t, 17> savedSlots{};
    std::memcpy(savedSlots.data(), thread->stack + top,
                requiredSlots * sizeof(std::uint64_t));

    for (std::size_t index = 0; index < argumentCount; ++index)
        thread->stack[context.stackPointer++] = arguments[index];
    thread->stack[context.stackPointer++] = 0;
    context.programCounter = programCounter;
    context.state = 0;

    void* previousThread = tls->currentScriptThread;
    const bool previousActive = tls->scriptThreadActive;
    tls->currentScriptThread = thread;
    tls->scriptThreadActive = true;

    (void)scriptVm(thread->stack,
                   reinterpret_cast<std::int64_t**>(g_runtime.scriptGlobalsAddress),
                   program,
                   &context);

    tls->scriptThreadActive = previousActive;
    tls->currentScriptThread = previousThread;

    if (result)
        *result = thread->stack[top];
    std::memcpy(thread->stack + top, savedSlots.data(),
                requiredSlots * sizeof(std::uint64_t));
    return true;
}

FreemodeRandomEventsData* ResolveFreemodeData(GTA_Script_Thread_View* thread) noexcept
{
    if (!thread || !thread->stack ||
        thread->context.stackSize < FreemodeRandomEventsLocal + 315U) {
        return nullptr;
    }

    auto* data = reinterpret_cast<FreemodeRandomEventsData*>(
        thread->stack + FreemodeRandomEventsLocal);
    if (!IsReadableAddress(reinterpret_cast<std::uintptr_t>(data), sizeof(*data)))
        return nullptr;
    return data;
}

bool ResolveRandomEventsContext(
    GTA_Native_Manager& natives,
    GTA_Random_Events_Snapshot& snapshot,
    ResolvedRandomEventsContext& resolved) noexcept
{
    auto* server = ResolveGlobal<GsbdRandomEvents>(GsbdRandomEventsGlobal);
    if (!server || server->eventArraySize.value != static_cast<int>(GTA_Random_Event_Count)) {
        snapshot.runtimeDetail = "Freemode random-event global block is not loaded.";
        return false;
    }

    const auto player = natives.Invoke<int>(GTA_Native_Id::PlayerId);
    if (!player || *player < 0 || *player >= 32) {
        snapshot.runtimeDetail = "The local GTA Online player is not available.";
        return false;
    }

    auto* gpbdSize = ResolveGlobal<std::int32_t>(GpbdFm2Global);
    if (!gpbdSize || *gpbdSize != 32) {
        snapshot.runtimeDetail = "Freemode player global block is not loaded.";
        return false;
    }

    const std::uint32_t clientBase = GpbdFm2Global + 1U +
        (static_cast<std::uint32_t>(*player) *
         static_cast<std::uint32_t>(GpbdFm2EntrySlots)) +
        static_cast<std::uint32_t>(RandomEventsClientOffset);
    auto* client = ResolveGlobal<RandomEventsClientData>(clientBase);
    if (!client || client->initState.value != 1 ||
        client->eventArraySize.value != static_cast<int>(GTA_Random_Event_Count) ||
        server->initState.value != 1) {
        snapshot.runtimeDetail = "Random Events are not initialized for the local player.";
        return false;
    }
    snapshot.clientInitialized = true;

    auto* freemodeThread = FindScriptThread(Joaat("freemode"));
    if (!freemodeThread) {
        snapshot.runtimeDetail = "Freemode is not running.";
        return false;
    }

    auto* freemode = ResolveFreemodeData(freemodeThread);
    if (!freemode ||
        freemode->eventArraySize.value != static_cast<int>(GTA_Random_Event_Count) ||
        freemode->fmmcArraySize.value != static_cast<int>(GTA_Random_Event_Count)) {
        snapshot.runtimeDetail = "The freemode random-event stack is not valid.";
        return false;
    }

    snapshot.freemodeRunning = true;
    snapshot.runtimeDetail = "Random Events runtime is ready.";
    resolved.server = server;
    resolved.client = client;
    resolved.freemode = freemode;
    resolved.freemodeThread = freemodeThread;
    resolved.player = *player;
    return true;
}

int RemainingTime(
    const RandomEventServerData& event,
    int duration,
    const std::optional<int>& networkTime) noexcept
{
    if (duration <= 0)
        return 0;
    if (!networkTime || event.timer.initialized.value == 0)
        return duration;

    const auto now = static_cast<std::uint32_t>(*networkTime);
    const auto started = static_cast<std::uint32_t>(event.timer.time.value);
    const auto elapsed = static_cast<std::uint32_t>(now - started);
    if (elapsed >= static_cast<std::uint32_t>(duration))
        return 0;
    return duration - static_cast<int>(elapsed);
}

GTA_Random_Event_State DecodeState(int state) noexcept
{
    switch (state) {
    case 0: return GTA_Random_Event_State::Inactive;
    case 1: return GTA_Random_Event_State::Available;
    case 2: return GTA_Random_Event_State::Active;
    case 3: return GTA_Random_Event_State::Cleanup;
    default: return GTA_Random_Event_State::Unknown;
    }
}

bool HasCoordinates(const ScriptVector3& position) noexcept
{
    // Consume only coordinates freemode has already published. The upstream
    // bytecode patch that forces coordinate updates is intentionally omitted:
    // its own implementation marks that patch as capable of crashing GTA.
    const float x = position.x.value;
    const float y = position.y.value;
    const float z = position.z.value;
    return std::isfinite(x) && std::isfinite(y) && std::isfinite(z) &&
           (std::fabs(x) + std::fabs(y) + std::fabs(z)) > 0.001F;
}

int QueryMaxSubvariation(
    GTA_Script_Thread_View* thread,
    GTA_Script_Program_View* program,
    int fmmcType) noexcept
{
    if (!thread || !program)
        return -1;

    if (g_runtime.freemodeProgram != program) {
        g_runtime.freemodeProgram = program;
        g_runtime.getNumVariationsPc = 0;
        g_runtime.getNumVariationsScanned = false;
        g_runtime.maxSubvariations.fill(-1);
        g_runtime.variationQueried.fill(false);
    }

    if (!g_runtime.getNumVariationsScanned) {
        g_runtime.getNumVariationsPc = FindScriptFunctionPc(program, GetNumVariationsPattern);
        g_runtime.getNumVariationsScanned = true;
    }
    if (g_runtime.getNumVariationsPc == 0)
        return -1;

    const std::array<std::uint64_t, 2> arguments{
        static_cast<std::uint64_t>(static_cast<std::uint32_t>(fmmcType)), 0
    };
    std::uint64_t rawResult = 0;
    if (!CallScriptFunction(thread, program, g_runtime.getNumVariationsPc,
                            arguments.data(), arguments.size(), &rawResult)) {
        return -1;
    }

    const int count = static_cast<int>(static_cast<std::uint32_t>(rawResult));
    if (count <= 0 || count > 256)
        return -1;
    return count - 1;
}

bool IsLocalScriptHost(const GTA_Script_Thread_View& thread, bool& localHost) noexcept
{
    const auto netComponent = reinterpret_cast<std::uintptr_t>(thread.netComponent);
    if (!IsReadableAddress(netComponent, 0x3A))
        return false;

    std::uintptr_t host = 0;
    std::int16_t localSlot = -1;
    std::memcpy(&host, reinterpret_cast<const void*>(netComponent + 0x30), sizeof(host));
    std::memcpy(&localSlot, reinterpret_cast<const void*>(netComponent + 0x38), sizeof(localSlot));
    if (host == 0) {
        localHost = true;
        return true;
    }
    if (localSlot < 0 || localSlot >= 32)
        return false;
    if (!IsReadableAddress(host + 0x1A, sizeof(std::uint16_t)))
        return false;

    std::uint16_t hostSlot = 0;
    std::memcpy(&hostSlot, reinterpret_cast<const void*>(host + 0x1A), sizeof(hostSlot));
    if (hostSlot >= 32)
        return false;
    localHost = hostSlot == static_cast<std::uint16_t>(localSlot);
    return true;
}

bool KillActiveRandomEvent(std::size_t eventIndex, std::string& detail) noexcept
{
    const auto scriptHash = Joaat(EventScriptNames[eventIndex]);
    auto* thread = FindScriptThread(scriptHash);
    auto* program = FindScriptProgram(scriptHash);
    if (!thread || !program) {
        detail = "Event script is not active; the local player may not be a participant.";
        return false;
    }

    bool localHost = false;
    if (!IsLocalScriptHost(*thread, localHost)) {
        detail = "Event host state could not be validated.";
        return false;
    }

    if (g_runtime.killPrograms[eventIndex] != program) {
        g_runtime.killPrograms[eventIndex] = program;
        g_runtime.setServerStatePcs[eventIndex] = 0;
        g_runtime.setClientStatePcs[eventIndex] = 0;
    }

    auto& programCounter = localHost
        ? g_runtime.setServerStatePcs[eventIndex]
        : g_runtime.setClientStatePcs[eventIndex];
    if (programCounter == 0) {
        programCounter = localHost
            ? FindScriptFunctionPc(program, SetServerStatePattern)
            : FindScriptFunctionPc(program, SetClientStatePattern);
    }
    if (programCounter == 0) {
        detail = "The event cleanup function was not found for this script version.";
        return false;
    }

    const std::uint64_t cleanupState = 3;
    if (!CallScriptFunction(thread, program, programCounter, &cleanupState, 1)) {
        detail = "The event cleanup function could not be invoked.";
        return false;
    }

    detail = localHost
        ? "Server cleanup was requested for the active event."
        : "Client cleanup was requested for the active event.";
    return true;
}

bool TeleportCurrentEntity(
    GTA_Native_Manager& natives,
    const ScriptVector3& position) noexcept
{
    const auto ped = natives.Invoke<int>(GTA_Native_Id::PlayerPedId);
    if (!ped || *ped == 0)
        return false;

    int entity = *ped;
    const auto seated = natives.Invoke<bool>(GTA_Native_Id::IsPedInAnyVehicle, *ped, false);
    if (seated && *seated) {
        const auto vehicle = natives.Invoke<int>(GTA_Native_Id::GetVehiclePedIsIn, *ped, false);
        if (vehicle && *vehicle != 0)
            entity = *vehicle;
    }

    return natives.Invoke<void>(
        GTA_Native_Id::SetEntityCoordsNoOffset,
        entity,
        position.x.value,
        position.y.value,
        position.z.value,
        true,
        true,
        true);
}

RandomEventActionResult ExecuteCommand(
    GTA_Native_Manager& natives,
    const ResolvedRandomEventsContext& context,
    const RandomEventCommand& command) noexcept
{
    const auto eventIndex = static_cast<std::size_t>(command.event);
    if (eventIndex >= GTA_Random_Event_Count) {
        return {GTA_Random_Event_Action_Status::Failed, "The selected event is invalid."};
    }

    auto& serverEvent = context.server->events[eventIndex];
    auto& freemodeEvent = context.freemode->events[eventIndex];
    const auto state = DecodeState(serverEvent.state.value);

    switch (command.type) {
    case RandomEventCommandType::Launch: {
        if (state == GTA_Random_Event_State::Active) {
            return {GTA_Random_Event_Action_Status::AlreadyActive,
                    "The selected event is already active."};
        }

        const int knownMaximum = g_runtime.maxSubvariations[eventIndex];
        if (command.value < 0 || command.value > 255 ||
            (knownMaximum >= 0 && command.value > knownMaximum)) {
            return {GTA_Random_Event_Action_Status::Failed,
                    "The selected location is outside the event's variation range."};
        }

        RequestRandomEventPayload payload{};
        payload.eventIndex.value = RequestRandomEventIndex;
        payload.fmmcType.value = context.freemode->fmmcTypes[eventIndex].value;
        payload.subvariation.value = command.value;
        payload.playersToSend.value = 1;

        const bool sent = natives.InvokeHash<void>(
            SendTuScriptEventHash,
            1,
            &payload,
            static_cast<int>(sizeof(payload) / sizeof(std::uint64_t)),
            payload.playerBits.value,
            static_cast<std::uint32_t>(payload.eventIndex.value));
        if (!sent) {
            return {GTA_Random_Event_Action_Status::Failed,
                    "The GTA random-event request handler is unavailable."};
        }
        return {GTA_Random_Event_Action_Status::RequestSent,
                "Launch request sent. Freemode script host approval is required."};
    }
    case RandomEventCommandType::Kill: {
        if (state == GTA_Random_Event_State::Available) {
            if (!IsWritableAddress(reinterpret_cast<std::uintptr_t>(&serverEvent.state),
                                   sizeof(serverEvent.state))) {
                return {GTA_Random_Event_Action_Status::Failed,
                        "The event state is not writable."};
            }
            serverEvent.state.value = 3;
            return {GTA_Random_Event_Action_Status::Succeeded,
                    "Cleanup was requested for the available event."};
        }
        if (state == GTA_Random_Event_State::Active) {
            std::string detail;
            const bool killed = KillActiveRandomEvent(eventIndex, detail);
            return {killed ? GTA_Random_Event_Action_Status::Succeeded
                           : GTA_Random_Event_Action_Status::Failed,
                    std::move(detail)};
        }
        return {GTA_Random_Event_Action_Status::NotActive,
                "The selected event is not available or active."};
    }
    case RandomEventCommandType::Teleport:
        if (state == GTA_Random_Event_State::Inactive ||
            state == GTA_Random_Event_State::Unknown) {
            return {GTA_Random_Event_Action_Status::NotActive,
                    "The selected event is not active."};
        }
        if (!HasCoordinates(serverEvent.triggerPosition)) {
            return {GTA_Random_Event_Action_Status::CoordinatesUnavailable,
                    "Event coordinates are not available yet."};
        }
        if (!TeleportCurrentEntity(natives, serverEvent.triggerPosition)) {
            return {GTA_Random_Event_Action_Status::Failed,
                    "The player or current vehicle could not be teleported."};
        }
        return {GTA_Random_Event_Action_Status::Succeeded,
                "Teleported to the event trigger position."};

    case RandomEventCommandType::SetCooldown:
        if (command.value < 0 ||
            !IsWritableAddress(reinterpret_cast<std::uintptr_t>(&freemodeEvent.inactiveTime),
                               sizeof(freemodeEvent.inactiveTime))) {
            return {GTA_Random_Event_Action_Status::Failed,
                    "The event cooldown value is invalid or not writable."};
        }
        freemodeEvent.inactiveTime.value = command.value;
        return {GTA_Random_Event_Action_Status::Succeeded,
                "Event cooldown updated for the running freemode script."};

    case RandomEventCommandType::SetAvailability:
        if (command.value < 0 ||
            !IsWritableAddress(reinterpret_cast<std::uintptr_t>(&freemodeEvent.availableTime),
                               sizeof(freemodeEvent.availableTime))) {
            return {GTA_Random_Event_Action_Status::Failed,
                    "The event availability value is invalid or not writable."};
        }
        freemodeEvent.availableTime.value = command.value;
        return {GTA_Random_Event_Action_Status::Succeeded,
                "Event availability updated for the running freemode script."};
    }

    return {GTA_Random_Event_Action_Status::Failed, "Unknown random-event command."};
}

std::optional<RandomEventCommand> TakePendingCommand() noexcept
{
    std::scoped_lock lock(g_stateMutex);
    if (!g_pendingCommand)
        return std::nullopt;
    auto command = g_pendingCommand;
    g_pendingCommand.reset();
    g_commandInFlight = true;
    return command;
}

void PublishSnapshot(
    GTA_Random_Events_Snapshot runtimeSnapshot,
    const std::optional<RandomEventActionResult>& actionResult) noexcept
{
    std::scoped_lock lock(g_stateMutex);
    g_snapshot.runtimeReady = runtimeSnapshot.runtimeReady;
    g_snapshot.freemodeRunning = runtimeSnapshot.freemodeRunning;
    g_snapshot.clientInitialized = runtimeSnapshot.clientInitialized;
    g_snapshot.activeEventCount = runtimeSnapshot.activeEventCount;
    g_snapshot.events = std::move(runtimeSnapshot.events);
    g_snapshot.runtimeDetail = std::move(runtimeSnapshot.runtimeDetail);

    if (actionResult) {
        g_snapshot.actionStatus = actionResult->status;
        g_snapshot.actionDetail = actionResult->detail;
        g_commandInFlight = false;
    } else if (g_snapshot.runtimeReady && g_snapshot.freemodeRunning &&
               g_snapshot.clientInitialized && !g_pendingCommand && !g_commandInFlight &&
               g_snapshot.actionStatus == GTA_Random_Event_Action_Status::Unavailable) {
        g_snapshot.actionStatus = GTA_Random_Event_Action_Status::Ready;
        g_snapshot.actionDetail = "Ready.";
    }
}

bool QueueCommand(RandomEventCommand command) noexcept
{
    if (!ValidEvent(command.event))
        return false;

    std::scoped_lock lock(g_stateMutex);
    if (!g_snapshot.runtimeReady || !g_snapshot.freemodeRunning ||
        !g_snapshot.clientInitialized) {
        g_snapshot.actionStatus = GTA_Random_Event_Action_Status::Unavailable;
        g_snapshot.actionDetail = "Random Events runtime is not ready.";
        return false;
    }
    if (g_pendingCommand || g_commandInFlight)
        return false;

    g_pendingCommand = command;
    g_snapshot.actionStatus = GTA_Random_Event_Action_Status::Queued;
    g_snapshot.actionDetail = "Action queued on the GTA script thread.";
    return true;
}
}

const char* GTA_Random_Event_Name(GTA_Random_Event_Id event) noexcept
{
    const auto index = static_cast<std::size_t>(event);
    return index < EventNames.size() ? EventNames[index] : "Unknown Event";
}

GTA_Random_Events_Snapshot GetRandomEventsSnapshot()
{
    std::scoped_lock lock(g_stateMutex);
    return g_snapshot;
}

void SelectRandomEvent(GTA_Random_Event_Id event) noexcept
{
    if (ValidEvent(event))
        g_selectedEvent.store(static_cast<std::size_t>(event), std::memory_order_release);
}

bool RequestLaunchRandomEvent(GTA_Random_Event_Id event, int subvariation) noexcept
{
    return QueueCommand({RandomEventCommandType::Launch, event, subvariation});
}

bool RequestKillRandomEvent(GTA_Random_Event_Id event) noexcept
{
    return QueueCommand({RandomEventCommandType::Kill, event, 0});
}

bool RequestTeleportToRandomEvent(GTA_Random_Event_Id event) noexcept
{
    return QueueCommand({RandomEventCommandType::Teleport, event, 0});
}

bool RequestSetRandomEventCooldown(GTA_Random_Event_Id event, int milliseconds) noexcept
{
    return QueueCommand({RandomEventCommandType::SetCooldown, event, milliseconds});
}

bool RequestSetRandomEventAvailability(GTA_Random_Event_Id event, int milliseconds) noexcept
{
    return QueueCommand({RandomEventCommandType::SetAvailability, event, milliseconds});
}

void ConfigureRandomEventsExtension(
    std::uintptr_t scriptGlobalsAddress,
    std::uintptr_t programTableAddress,
    std::uintptr_t scriptThreadsStorageAddress,
    std::uintptr_t scriptVmAddress,
    std::uint64_t buildFingerprint,
    Backend::LoggerService* logger) noexcept
{
    ResetRandomEventsExtension();

    g_runtime.scriptGlobalsAddress = scriptGlobalsAddress;
    g_runtime.programTableAddress = programTableAddress;
    g_runtime.scriptThreadsStorageAddress = scriptThreadsStorageAddress;
    g_runtime.scriptVmAddress = scriptVmAddress;
    g_runtime.buildFingerprint = buildFingerprint;
    g_runtime.logger = logger;
    g_runtime.maxSubvariations.fill(-1);
    g_runtime.ready = buildFingerprint == SupportedBuildFingerprint &&
                      scriptGlobalsAddress != 0 && programTableAddress != 0 &&
                      scriptThreadsStorageAddress != 0 && scriptVmAddress != 0;

    {
        std::scoped_lock lock(g_stateMutex);
        g_snapshot.runtimeReady = g_runtime.ready;
        g_snapshot.runtimeDetail = g_runtime.ready
            ? "Waiting for freemode random-event state."
            : (buildFingerprint != SupportedBuildFingerprint
                   ? "Random Events offsets are not verified for this GTA build."
                   : "ScriptGlobals, ProgramTable, ScriptThreads, or ScriptVM is unavailable.");
    }

    if (logger) {
        logger->Log(
            g_runtime.ready ? Backend::LogLevel::Info : Backend::LogLevel::Warning,
            g_runtime.ready
                ? "Random Events ready | verified Enhanced globals, locals, ScriptVM, and event native enabled"
                : "Random Events unavailable | build fingerprint or script runtime targets are not verified",
            "GTA5_Enhanced.RandomEvents");
    }
}

void ResetRandomEventsExtension() noexcept
{
    g_runtime = {};
    g_runtime.maxSubvariations.fill(-1);
    g_selectedEvent.store(0, std::memory_order_release);

    std::scoped_lock lock(g_stateMutex);
    g_snapshot = {};
    g_pendingCommand.reset();
    g_commandInFlight = false;
}

void TickRandomEventsExtension(GTA_Native_Manager& natives) noexcept
{
    GTA_Random_Events_Snapshot next{};
    next.runtimeReady = g_runtime.ready && natives.Ready();
    for (std::size_t index = 0; index < GTA_Random_Event_Count; ++index) {
        const int cachedMaximum = g_runtime.maxSubvariations[index];
        next.events[index].maxSubvariation = cachedMaximum >= 0 ? cachedMaximum : 29;
    }

    ResolvedRandomEventsContext context{};
    bool contextReady = false;
    if (!next.runtimeReady) {
        next.runtimeDetail = "Random Events runtime is unavailable.";
    } else {
        contextReady = ResolveRandomEventsContext(natives, next, context);
    }

    const auto selected = (std::min)(
        g_selectedEvent.load(std::memory_order_acquire),
        GTA_Random_Event_Count - 1);
    if (contextReady) {
        if (g_runtime.freemodeThread != context.freemodeThread ||
            g_runtime.freemodeThreadId != context.freemodeThread->context.threadId) {
            g_runtime.freemodeThread = context.freemodeThread;
            g_runtime.freemodeThreadId = context.freemodeThread->context.threadId;
            g_runtime.freemodeProgram = nullptr;
            g_runtime.getNumVariationsPc = 0;
            g_runtime.getNumVariationsScanned = false;
            g_runtime.maxSubvariations.fill(-1);
            g_runtime.variationQueried.fill(false);
        }
        if (!g_runtime.variationQueried[selected]) {
            if (auto* freemodeProgram = FindScriptProgram(Joaat("freemode"))) {
                g_runtime.maxSubvariations[selected] = QueryMaxSubvariation(
                    context.freemodeThread,
                    freemodeProgram,
                    context.freemode->fmmcTypes[selected].value);
                g_runtime.variationQueried[selected] = true;
            }
        }
    }

    auto command = TakePendingCommand();
    std::optional<RandomEventActionResult> actionResult;
    if (command) {
        actionResult = contextReady
            ? ExecuteCommand(natives, context, *command)
            : RandomEventActionResult{
                  GTA_Random_Event_Action_Status::Failed,
                  "The action was cancelled because freemode Random Events are not ready."};
    }

    if (contextReady) {
        const auto networkTime = natives.InvokeHash<int>(GetNetworkTimeHash);
        for (std::size_t index = 0; index < GTA_Random_Event_Count; ++index) {
            auto& output = next.events[index];
            const auto& serverEvent = context.server->events[index];
            const auto& freemodeEvent = context.freemode->events[index];

            output.state = DecodeState(serverEvent.state.value);
            output.subvariation = serverEvent.subvariation.value;
            output.maxSubvariation = g_runtime.maxSubvariations[index] >= 0
                ? g_runtime.maxSubvariations[index]
                : 29;
            output.fmmcType = context.freemode->fmmcTypes[index].value;
            output.inactiveTimeMs = freemodeEvent.inactiveTime.value;
            output.availableTimeMs = freemodeEvent.availableTime.value;
            output.triggerX = serverEvent.triggerPosition.x.value;
            output.triggerY = serverEvent.triggerPosition.y.value;
            output.triggerZ = serverEvent.triggerPosition.z.value;
            output.triggerRange = serverEvent.triggerRange.value;
            output.hasCoordinates = HasCoordinates(serverEvent.triggerPosition);

            if (output.state == GTA_Random_Event_State::Inactive) {
                output.remainingTimeMs = RemainingTime(
                    serverEvent, output.inactiveTimeMs, networkTime);
            } else if (output.state == GTA_Random_Event_State::Available) {
                output.remainingTimeMs = RemainingTime(
                    serverEvent, output.availableTimeMs, networkTime);
            }

            if (context.client->events[index].state.value != 0)
                ++next.activeEventCount;

            if (index == selected && output.state == GTA_Random_Event_State::Active) {
                output.scriptRunning =
                    FindScriptThread(Joaat(EventScriptNames[index])) != nullptr;
            }
        }
    }

    PublishSnapshot(std::move(next), actionResult);

    if (actionResult && g_runtime.logger) {
        const bool success = actionResult->status == GTA_Random_Event_Action_Status::Succeeded ||
                             actionResult->status == GTA_Random_Event_Action_Status::RequestSent;
        g_runtime.logger->Log(
            success ? Backend::LogLevel::Info : Backend::LogLevel::Warning,
            actionResult->detail,
            "GTA5_Enhanced.RandomEvents");
    }
}
}
