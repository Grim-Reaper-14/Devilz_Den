#include "GTA_Vehicle_Personal_Save.hpp"

#include "Backend/Logging/LoggerService.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Manager.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Registry.hpp"
#include "GTA_Self_Online_Extension.hpp"

#include <Windows.h>
#include <intrin.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace Devilz::Integrations::GTA5_Enhanced
{
namespace
{
constexpr std::size_t ScriptProgramCount = 176;
constexpr std::size_t ScriptCodePageSize = 0x4000;
constexpr std::size_t VehicleRewardDataLocal = 148;
constexpr std::size_t VehicleMenuDataLocal = 195;
constexpr std::uint32_t FreemodeGeneralBase = 2733326U;
constexpr std::uint32_t FreemodePersonalVehicleIndexOffset = 301U;

constexpr std::array<int, 11> IsVehicleValidForPvPattern{
    0x5D, -1, -1, -1, 0x2A, 0x06, 0x56, 0x13, 0x00, 0x38, 0x00
};
constexpr std::array<int, 5> GiveVehicleRewardPattern{
    0x2D, 0x0C, 0x1E, 0x00, 0x00
};

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

struct GTA_Vehicle_Reward_Data_View
{
    std::uint64_t pad0000[3]{};
    alignas(8) int transactionStatus = 0;
    alignas(8) int garage = 0;
    alignas(8) int garageOffset = 0;
    alignas(8) int controlStatus = 0;
    std::uint64_t pad0008[40]{};
};
static_assert(sizeof(GTA_Vehicle_Reward_Data_View) == 47 * sizeof(std::uint64_t));

using ScriptVm = int (*)(std::uint64_t* stack, std::int64_t** scriptGlobals,
                         GTA_Script_Program_View* program, void* context);

struct VehiclePersonalSaveRuntime
{
    std::uintptr_t scriptGlobalsAddress = 0;
    std::uintptr_t programTableAddress = 0;
    std::uintptr_t scriptThreadsStorageAddress = 0;
    std::uintptr_t scriptVmAddress = 0;
    Backend::LoggerService* logger = nullptr;
    std::uint32_t isVehicleValidForPvPc = 0;
    std::uint32_t giveVehicleRewardPc = 0;
};

VehiclePersonalSaveRuntime g_runtime{};
std::atomic_bool g_requestPending{false};
std::atomic<GTA_Vehicle_Personal_Save_Status> g_status{GTA_Vehicle_Personal_Save_Status::Idle};

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
    const bool readable = protection == PAGE_READONLY || protection == PAGE_READWRITE ||
                          protection == PAGE_WRITECOPY || protection == PAGE_EXECUTE_READ ||
                          protection == PAGE_EXECUTE_READWRITE || protection == PAGE_EXECUTE_WRITECOPY;
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

void Log(Backend::LogLevel level, std::string message)
{
    if (!g_runtime.logger)
        return;

    Backend::LogContext context{};
    context.service = "GTA5_Enhanced.Vehicle.Personal.Save";
    context.threadName = "GameThread";
    g_runtime.logger->LogWithContext(level, std::move(message), std::move(context));
}

template <typename T>
T* ResolveGlobal(std::uint32_t index) noexcept
{
    constexpr std::size_t GlobalBlockCount = 64;
    const std::uint32_t blockIndex = (index >> 0x12U) & 0x3FU;
    const std::uint32_t slotIndex = index & 0x3FFFFU;
    if (blockIndex >= GlobalBlockCount ||
        !IsReadableAddress(g_runtime.scriptGlobalsAddress, GlobalBlockCount * sizeof(std::int64_t*))) {
        return nullptr;
    }

    auto** globals = reinterpret_cast<std::int64_t**>(g_runtime.scriptGlobalsAddress);
    std::int64_t* block = nullptr;
    std::memcpy(&block, globals + blockIndex, sizeof(block));
    if (!block)
        return nullptr;

    auto* value = reinterpret_cast<T*>(block + slotIndex);
    return IsReadableAddress(reinterpret_cast<std::uintptr_t>(value), sizeof(T)) ? value : nullptr;
}

GTA_Script_Thread_View* FindScriptThread(std::uint32_t scriptHash) noexcept
{
    if (!IsReadableAddress(g_runtime.scriptThreadsStorageAddress, sizeof(GTA_Script_Thread_Array_View)))
        return nullptr;

    GTA_Script_Thread_Array_View threads{};
    std::memcpy(&threads,
                reinterpret_cast<const void*>(g_runtime.scriptThreadsStorageAddress),
                sizeof(threads));

    const auto count = (std::min<std::size_t>)(threads.size, 256U);
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

bool ReadProgramByte(const GTA_Script_Program_View* program, std::uint32_t pc, std::uint8_t& value) noexcept
{
    if (!program || !program->codeBlocks || pc >= program->codeSize)
        return false;

    const std::size_t pageCount = (program->codeSize + ScriptCodePageSize - 1U) / ScriptCodePageSize;
    if (!IsReadableAddress(reinterpret_cast<std::uintptr_t>(program->codeBlocks), pageCount * sizeof(std::uint8_t*)))
        return false;

    const std::size_t pageIndex = pc / ScriptCodePageSize;
    const std::size_t pageOffset = pc % ScriptCodePageSize;
    if (pageIndex >= pageCount)
        return false;

    std::uint8_t* page = nullptr;
    std::memcpy(&page, program->codeBlocks + pageIndex, sizeof(page));
    if (!page || !IsReadableAddress(reinterpret_cast<std::uintptr_t>(page + pageOffset), 1U))
        return false;

    value = page[pageOffset];
    return true;
}

template <std::size_t N>
std::uint32_t FindUniquePatternPc(const GTA_Script_Program_View* program, const std::array<int, N>& pattern) noexcept
{
    if (!program || program->codeSize < N)
        return 0;

    std::uint32_t found = 0;
    std::size_t matches = 0;
    for (std::uint32_t pc = 0; pc + N <= program->codeSize; ++pc) {
        bool match = true;
        for (std::size_t offset = 0; offset < N; ++offset) {
            if (pattern[offset] < 0)
                continue;
            std::uint8_t value = 0;
            if (!ReadProgramByte(program, pc + static_cast<std::uint32_t>(offset), value) ||
                value != static_cast<std::uint8_t>(pattern[offset])) {
                match = false;
                break;
            }
        }
        if (!match)
            continue;
        found = pc;
        if (++matches > 1U)
            return 0;
    }
    return matches == 1U ? found : 0U;
}

std::uint32_t ReadProgramU24(const GTA_Script_Program_View* program, std::uint32_t pc) noexcept
{
    std::uint8_t bytes[3]{};
    if (!ReadProgramByte(program, pc, bytes[0]) ||
        !ReadProgramByte(program, pc + 1U, bytes[1]) ||
        !ReadProgramByte(program, pc + 2U, bytes[2])) {
        return 0;
    }
    return static_cast<std::uint32_t>(bytes[0]) |
           (static_cast<std::uint32_t>(bytes[1]) << 8U) |
           (static_cast<std::uint32_t>(bytes[2]) << 16U);
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

template <typename T>
std::uint64_t PackScriptArg(T value) noexcept
{
    static_assert(std::is_trivially_copyable_v<T>);
    static_assert(sizeof(T) <= sizeof(std::uint64_t));
    std::uint64_t packed = 0;
    std::memcpy(&packed, &value, sizeof(T));
    return packed;
}

std::optional<std::uint64_t> CallScriptFunctionRaw(
    GTA_Script_Thread_View* thread,
    GTA_Script_Program_View* program,
    std::uint32_t pc,
    std::span<const std::uint64_t> args) noexcept
{
    if (!thread || !program || !thread->stack || pc == 0 ||
        g_runtime.scriptVmAddress == 0 || g_runtime.scriptGlobalsAddress == 0)
        return std::nullopt;

    auto* tls = CurrentTls();
    if (!tls)
        return std::nullopt;

    const auto scriptVm = reinterpret_cast<ScriptVm>(g_runtime.scriptVmAddress);
    if (!scriptVm || !IsReadableAddress(g_runtime.scriptVmAddress, 1U))
        return std::nullopt;

    auto context = thread->context;
    const auto topStack = context.stackPointer;
    const std::size_t requiredSlots = args.size() + 1U;
    if (!IsWritableAddress(
            reinterpret_cast<std::uintptr_t>(thread->stack + topStack),
            requiredSlots * sizeof(std::uint64_t))) {
        return std::nullopt;
    }

    for (const auto arg : args)
        thread->stack[context.stackPointer++] = arg;
    thread->stack[context.stackPointer++] = 0;
    context.programCounter = pc;
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

    if (!IsReadableAddress(reinterpret_cast<std::uintptr_t>(thread->stack + topStack), sizeof(std::uint64_t)))
        return std::nullopt;
    return thread->stack[topStack];
}

std::optional<bool> IsVehicleValidForPersonalVehicle(std::uint32_t modelHash) noexcept
{
    const auto freemodeHash = Joaat("freemode");
    auto* thread = FindScriptThread(freemodeHash);
    auto* program = FindScriptProgram(freemodeHash);
    if (!thread || !program)
        return std::nullopt;

    if (g_runtime.isVehicleValidForPvPc == 0) {
        const auto match = FindUniquePatternPc(program, IsVehicleValidForPvPattern);
        if (match == 0)
            return std::nullopt;
        g_runtime.isVehicleValidForPvPc = ReadProgramU24(program, match + 1U);
    }
    if (g_runtime.isVehicleValidForPvPc == 0)
        return std::nullopt;

    const std::array<std::uint64_t, 1> args{PackScriptArg(modelHash)};
    const auto result = CallScriptFunctionRaw(thread, program, g_runtime.isVehicleValidForPvPc, args);
    return result ? std::optional<bool>{(*result & 0xFFU) != 0} : std::nullopt;
}

bool IsBlacklistedModel(std::uint32_t modelHash) noexcept
{
    static const std::array<std::uint32_t, 7> blacklisted{
        Joaat("rcbandito"), Joaat("minitank"), Joaat("thruster"), Joaat("terbyte"),
        Joaat("avenger"), Joaat("policet3"), Joaat("brickade2")
    };
    return std::find(blacklisted.begin(), blacklisted.end(), modelHash) != blacklisted.end();
}

bool DriveGarageReward(int vehicle) noexcept
{
    const auto rewardHash = Joaat("AM_MP_VEHICLE_REWARD");
    auto* thread = FindScriptThread(rewardHash);
    auto* program = FindScriptProgram(rewardHash);
    if (!thread || !program || !thread->stack) {
        g_status.store(GTA_Vehicle_Personal_Save_Status::Unavailable, std::memory_order_release);
        Log(Backend::LogLevel::Warning,
            "Save Personal Vehicle unavailable: AM_MP_VEHICLE_REWARD is not active; return to GTA Online freemode");
        return false;
    }

    if (g_runtime.giveVehicleRewardPc == 0)
        g_runtime.giveVehicleRewardPc = FindUniquePatternPc(program, GiveVehicleRewardPattern);
    if (g_runtime.giveVehicleRewardPc == 0) {
        g_status.store(GTA_Vehicle_Personal_Save_Status::Unavailable, std::memory_order_release);
        Log(Backend::LogLevel::Warning,
            "Save Personal Vehicle unavailable: GiveVehicleReward bytecode signature was not resolved uniquely");
        return false;
    }

    auto* rewardData = reinterpret_cast<GTA_Vehicle_Reward_Data_View*>(thread->stack + VehicleRewardDataLocal);
    auto* vehicleMenuData = reinterpret_cast<int*>(thread->stack + VehicleMenuDataLocal);
    if (!IsWritableAddress(reinterpret_cast<std::uintptr_t>(rewardData), sizeof(*rewardData)) ||
        !IsWritableAddress(reinterpret_cast<std::uintptr_t>(vehicleMenuData), sizeof(int))) {
        g_status.store(GTA_Vehicle_Personal_Save_Status::Failed, std::memory_order_release);
        Log(Backend::LogLevel::Warning,
            "Save Personal Vehicle failed: AM_MP_VEHICLE_REWARD locals are not writable");
        return false;
    }

    const std::array<std::uint64_t, 12> args{
        PackScriptArg(vehicle),
        PackScriptArg(vehicleMenuData),
        PackScriptArg(&rewardData->transactionStatus),
        PackScriptArg(&rewardData->garage),
        PackScriptArg(&rewardData->garageOffset),
        PackScriptArg(&rewardData->controlStatus),
        PackScriptArg(false),
        PackScriptArg(true),
        PackScriptArg(true),
        PackScriptArg(false),
        PackScriptArg(0),
        PackScriptArg(-1)
    };

    g_status.store(GTA_Vehicle_Personal_Save_Status::OpeningGarageMenu, std::memory_order_release);
    const auto result = CallScriptFunctionRaw(thread, program, g_runtime.giveVehicleRewardPc, args);
    if (!result) {
        g_status.store(GTA_Vehicle_Personal_Save_Status::Failed, std::memory_order_release);
        Log(Backend::LogLevel::Warning,
            "Save Personal Vehicle failed: ScriptVM call did not complete");
        return false;
    }

    const bool completedCall = (*result & 0xFFU) != 0;
    if (!completedCall)
        return true;

    if (rewardData->controlStatus == 3) {
        g_status.store(GTA_Vehicle_Personal_Save_Status::WaitingForGarageSelection, std::memory_order_release);
        return true;
    }

    rewardData->transactionStatus = 0;
    rewardData->garage = 0;
    rewardData->garageOffset = 0;
    rewardData->controlStatus = 0;
    g_status.store(GTA_Vehicle_Personal_Save_Status::Completed, std::memory_order_release);
    RequestPersonalVehicleListRefresh();
    Log(Backend::LogLevel::Info,
        "GTA personal-vehicle garage transaction closed | Vehicle: " + std::to_string(vehicle) +
            " | Owned vehicle refresh queued");
    return false;
}
}

void ConfigureVehiclePersonalSave(
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
    g_requestPending.store(false, std::memory_order_release);
    g_status.store(GTA_Vehicle_Personal_Save_Status::Idle, std::memory_order_release);

    const bool ready = VehiclePersonalSaveRuntimeReady();
    Log(ready ? Backend::LogLevel::Info : Backend::LogLevel::Warning,
        ready
            ? "Save Personal Vehicle ready | AM_MP_VEHICLE_REWARD ScriptVM path enabled"
            : "Save Personal Vehicle unavailable: ScriptGlobals, ProgramTable, ScriptThreads, or ScriptVM target missing");
}

void ResetVehiclePersonalSave() noexcept
{
    g_requestPending.store(false, std::memory_order_release);
    g_status.store(GTA_Vehicle_Personal_Save_Status::Idle, std::memory_order_release);
    g_runtime = {};
}

void RequestSaveCurrentVehicleToGarage() noexcept
{
    bool expected = false;
    if (!g_requestPending.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
        return;
    g_status.store(GTA_Vehicle_Personal_Save_Status::Queued, std::memory_order_release);
}

GTA_Vehicle_Personal_Save_Status VehiclePersonalSaveStatus() noexcept
{
    return g_status.load(std::memory_order_acquire);
}

bool VehiclePersonalSaveRuntimeReady() noexcept
{
    return g_runtime.scriptGlobalsAddress != 0 && g_runtime.programTableAddress != 0 &&
           g_runtime.scriptThreadsStorageAddress != 0 && g_runtime.scriptVmAddress != 0;
}

void TickVehiclePersonalSave(GTA_Native_Manager& natives) noexcept
{
    if (!g_requestPending.load(std::memory_order_acquire))
        return;

    if (!VehiclePersonalSaveRuntimeReady() || !natives.Ready()) {
        g_status.store(GTA_Vehicle_Personal_Save_Status::Unavailable, std::memory_order_release);
        g_requestPending.store(false, std::memory_order_release);
        Log(Backend::LogLevel::Warning,
            "Save Personal Vehicle request rejected: ScriptVM runtime is unavailable");
        return;
    }

    const auto ped = natives.Invoke<int>(GTA_Native_Id::PlayerPedId);
    std::optional<int> vehicle;
    if (ped)
        vehicle = natives.Invoke<int>(GTA_Native_Id::GetVehiclePedIsIn, *ped, false);
    if (!vehicle || *vehicle == 0) {
        g_status.store(GTA_Vehicle_Personal_Save_Status::NoVehicle, std::memory_order_release);
        g_requestPending.store(false, std::memory_order_release);
        Log(Backend::LogLevel::Warning,
            "Save Personal Vehicle request rejected: player is not in a vehicle");
        return;
    }

    const auto status = g_status.load(std::memory_order_acquire);
    if (status == GTA_Vehicle_Personal_Save_Status::Queued ||
        status == GTA_Vehicle_Personal_Save_Status::Validating) {
        g_status.store(GTA_Vehicle_Personal_Save_Status::Validating, std::memory_order_release);

        if (!FindScriptThread(Joaat("freemode"))) {
            g_status.store(GTA_Vehicle_Personal_Save_Status::Unavailable, std::memory_order_release);
            g_requestPending.store(false, std::memory_order_release);
            Log(Backend::LogLevel::Warning,
                "Save Personal Vehicle request rejected: GTA Online freemode is not active");
            return;
        }

        const auto model = natives.Invoke<int>(GTA_Native_Id::GetEntityModel, *vehicle);
        if (!model || *model == 0) {
            g_status.store(GTA_Vehicle_Personal_Save_Status::Failed, std::memory_order_release);
            g_requestPending.store(false, std::memory_order_release);
            return;
        }
        const auto modelHash = static_cast<std::uint32_t>(*model);

        const auto valid = IsVehicleValidForPersonalVehicle(modelHash);
        if (IsBlacklistedModel(modelHash) || !valid.has_value() || !*valid) {
            g_status.store(valid.has_value()
                    ? GTA_Vehicle_Personal_Save_Status::InvalidVehicle
                    : GTA_Vehicle_Personal_Save_Status::Unavailable,
                std::memory_order_release);
            g_requestPending.store(false, std::memory_order_release);
            Log(Backend::LogLevel::Warning,
                valid.has_value()
                    ? "Save Personal Vehicle rejected: GTA reports this model cannot be stored as a personal vehicle"
                    : "Save Personal Vehicle unavailable: freemode IsVehicleValidForPV signature/call failed");
            return;
        }

        if (auto* personalVehicle = ResolveGlobal<int>(FreemodeGeneralBase + FreemodePersonalVehicleIndexOffset);
            personalVehicle && *personalVehicle == *vehicle) {
            g_status.store(GTA_Vehicle_Personal_Save_Status::AlreadyPersonalVehicle, std::memory_order_release);
            g_requestPending.store(false, std::memory_order_release);
            Log(Backend::LogLevel::Notice,
                "Save Personal Vehicle skipped: current vehicle is already the active personal vehicle");
            return;
        }

        Log(Backend::LogLevel::Info,
            "Save Personal Vehicle validated | Vehicle: " + std::to_string(*vehicle) +
                " | Model: " + std::to_string(modelHash));
    }

    if (!DriveGarageReward(*vehicle))
        g_requestPending.store(false, std::memory_order_release);
}
}
