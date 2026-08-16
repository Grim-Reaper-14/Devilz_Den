#pragma once

#include "Backend/Logging/LoggerService.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace Devilz::Integrations::GTA5_Enhanced
{
enum class GTA_Script_Function_Locator_Kind : std::uint8_t
{
    FixedProgramCounter,
    UniqueBytePattern
};

enum class GTA_Script_Function_Invoke_Status : std::uint8_t
{
    Idle,
    Queued,
    Running,
    Executed,
    RuntimeUnavailable,
    UnsupportedBuild,
    InvalidDescriptor,
    ScriptProgramNotFound,
    ScriptThreadNotFound,
    ProgramCounterOutOfRange,
    SignatureMismatch,
    PatternNotUnique,
    StackUnavailable,
    ScriptVmUnavailable,
    Failed
};

struct GTA_Script_Function_Descriptor
{
    std::string label;
    std::string scriptName;
    std::string sourceReference;
    std::uint64_t expectedBuildFingerprint = 0;
    bool verifiedSynchronous = false;
    GTA_Script_Function_Locator_Kind locator = GTA_Script_Function_Locator_Kind::FixedProgramCounter;
    std::uint32_t programCounter = 0;
    std::ptrdiff_t entryAdjustment = 0;

    // Byte values are 0..255. -1 is a wildcard. A descriptor must contain
    // enough exact bytes to fail closed against a stale build/script.
    std::vector<std::int16_t> bytePattern;
};

struct GTA_Script_Function_Invoke_Snapshot
{
    std::uint64_t requestId = 0;
    GTA_Script_Function_Invoke_Status status = GTA_Script_Function_Invoke_Status::Idle;
    std::string label;
    std::string scriptName;
    std::uint32_t resolvedProgramCounter = 0;
    int scriptVmResult = 0;
    std::string detail;
};

void ConfigureScriptFunctionInvoker(
    std::uintptr_t scriptGlobalsAddress,
    std::uintptr_t programTableAddress,
    std::uintptr_t scriptThreadsStorageAddress,
    std::uintptr_t scriptVmAddress,
    std::uint64_t buildFingerprint,
    Backend::LoggerService* logger) noexcept;

void ResetScriptFunctionInvoker() noexcept;

// Thread-safe producer API for menu/backend callers. The actual ScriptVM call
// is never performed here; it is deferred to TickScriptFunctionInvoker(),
// which must run from the existing RunScriptThreads game-thread bridge.
// Returns 0 when another call is already queued; the original request remains intact.
[[nodiscard]] std::uint64_t RequestVerifiedZeroArgumentScriptFunction(
    GTA_Script_Function_Descriptor descriptor) noexcept;

[[nodiscard]] GTA_Script_Function_Invoke_Snapshot ScriptFunctionInvokeSnapshot();
[[nodiscard]] bool HasPendingScriptFunctionInvocation() noexcept;

// Game-thread consumer. Returns true when a queued request was consumed.
[[nodiscard]] bool TickScriptFunctionInvoker() noexcept;

[[nodiscard]] const char* GTA_Script_Function_Invoke_Status_Name(
    GTA_Script_Function_Invoke_Status status) noexcept;
}
