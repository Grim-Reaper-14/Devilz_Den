#pragma once

#include "Backend/Logging/LoggerService.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Devilz::Integrations::GTA5_Enhanced
{
enum class GTA_Script_Value_Type : std::uint8_t
{
    Void,
    Int32,
    UInt32,
    Int64,
    UInt64,
    Float,
    Bool,
    Hash32
};

struct GTA_Script_Value
{
    GTA_Script_Value_Type type = GTA_Script_Value_Type::Void;
    std::uint64_t slot = 0;

    [[nodiscard]] static GTA_Script_Value Int32(std::int32_t value) noexcept;
    [[nodiscard]] static GTA_Script_Value UInt32(std::uint32_t value) noexcept;
    [[nodiscard]] static GTA_Script_Value Int64(std::int64_t value) noexcept;
    [[nodiscard]] static GTA_Script_Value UInt64(std::uint64_t value) noexcept;
    [[nodiscard]] static GTA_Script_Value Float(float value) noexcept;
    [[nodiscard]] static GTA_Script_Value Bool(bool value) noexcept;
    [[nodiscard]] static GTA_Script_Value Hash32(std::uint32_t value) noexcept;

    [[nodiscard]] std::optional<std::int32_t> AsInt32() const noexcept;
    [[nodiscard]] std::optional<std::uint32_t> AsUInt32() const noexcept;
    [[nodiscard]] std::optional<std::int64_t> AsInt64() const noexcept;
    [[nodiscard]] std::optional<std::uint64_t> AsUInt64() const noexcept;
    [[nodiscard]] std::optional<float> AsFloat() const noexcept;
    [[nodiscard]] std::optional<bool> AsBool() const noexcept;
    [[nodiscard]] std::optional<std::uint32_t> AsHash32() const noexcept;
};

enum class GTA_Script_Function_Locator_Kind : std::uint8_t
{
    FixedProgramCounter,
    UniqueBytePattern,
    UniqueBytePatternU24Target
};

enum class GTA_Script_Function_Exposure : std::uint8_t
{
    Internal,
    UserFacing
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
    DescriptorNotFound,
    DescriptorNotUserFacing,
    QueueFull,
    ArgumentCountMismatch,
    ArgumentTypeMismatch,
    ScriptProgramNotFound,
    ScriptThreadNotFound,
    ScriptCodeSizeMismatch,
    ProgramCounterOutOfRange,
    SignatureMismatch,
    PatternNotFound,
    PatternNotUnique,
    StackUnavailable,
    NotOnGameThread,
    ScriptVmUnavailable,
    UnexpectedYield,
    Failed
};

struct GTA_Script_Function_Source_Metadata
{
    std::string repository;
    std::string revision;
    std::string path;
    std::string function;
};

struct GTA_Script_Function_Abi
{
    std::vector<GTA_Script_Value_Type> arguments;
    GTA_Script_Value_Type returnType = GTA_Script_Value_Type::Void;
};

struct GTA_Script_Function_Descriptor
{
    // Keep the original field order first so existing aggregate initializers
    // continue to compile while the descriptor grows richer.
    std::string label;
    std::string scriptName;
    std::string sourceReference;
    std::uint64_t expectedBuildFingerprint = 0;
    bool verifiedSynchronous = false;
    GTA_Script_Function_Locator_Kind locator =
        GTA_Script_Function_Locator_Kind::FixedProgramCounter;
    std::uint32_t programCounter = 0;
    std::ptrdiff_t entryAdjustment = 0;

    // Byte values are 0..255. -1 is a wildcard. A descriptor must contain
    // enough exact bytes to fail closed against a stale build/script.
    std::vector<std::int16_t> bytePattern;

    // Powerhouse metadata. New descriptors should provide a stable id and
    // structured decompile provenance. Legacy direct-call descriptors without
    // an id are assigned a deterministic compatibility id internally.
    std::string id;
    std::string category;
    GTA_Script_Function_Source_Metadata source;
    std::uint32_t expectedScriptCodeSize = 0; // 0 means signature-only verification.
    GTA_Script_Function_Exposure exposure = GTA_Script_Function_Exposure::Internal;
    std::uint8_t targetOperandOffset = 1;
    GTA_Script_Function_Abi abi;
};
struct GTA_Script_Function_Invoke_Snapshot
{
    std::uint64_t requestId = 0;
    GTA_Script_Function_Invoke_Status status = GTA_Script_Function_Invoke_Status::Idle;
    std::string descriptorId;
    std::string label;
    std::string scriptName;
    std::uint32_t signatureProgramCounter = 0;
    std::uint32_t resolvedProgramCounter = 0;
    bool cacheHit = false;
    int scriptVmResult = 0;
    std::optional<GTA_Script_Value> returnValue;
    std::string detail;
};

struct GTA_Script_Function_Invoker_Metrics
{
    std::uint64_t requestsQueued = 0;
    std::uint64_t requestsRejected = 0;
    std::uint64_t invocationsExecuted = 0;
    std::uint64_t invocationsFailed = 0;
    std::uint64_t cacheHits = 0;
    std::uint64_t cacheMisses = 0;
    std::uint64_t signatureRevalidations = 0;
    std::uint64_t patternScans = 0;
    std::size_t registeredDescriptors = 0;
    std::size_t pendingRequests = 0;
    std::size_t cachedDescriptors = 0;
};

void ConfigureScriptFunctionInvoker(
    std::uintptr_t scriptGlobalsAddress,
    std::uintptr_t programTableAddress,
    std::uintptr_t scriptThreadsStorageAddress,
    std::uintptr_t scriptVmAddress,
    std::uint64_t buildFingerprint,
    Backend::LoggerService* logger) noexcept;

void ResetScriptFunctionInvoker() noexcept;
[[nodiscard]] bool ScriptFunctionInvokerReady() noexcept;

// Descriptor registry. This API is C++-only; menu/Lua surfaces should invoke
// registered user-facing ids rather than accepting arbitrary PCs/patterns.
[[nodiscard]] bool RegisterScriptFunctionDescriptor(
    GTA_Script_Function_Descriptor descriptor) noexcept;
[[nodiscard]] bool UnregisterScriptFunctionDescriptor(std::string_view descriptorId) noexcept;
[[nodiscard]] std::optional<GTA_Script_Function_Descriptor> FindScriptFunctionDescriptor(
    std::string_view descriptorId);
[[nodiscard]] std::vector<GTA_Script_Function_Descriptor> RegisteredScriptFunctions(
    bool userFacingOnly = false);
void ClearScriptFunctionRegistry() noexcept;
void ClearScriptFunctionPcCache() noexcept;

// Thread-safe producer APIs. The actual ScriptVM call is deferred to
// TickScriptFunctionInvoker(), which runs from the existing RunScriptThreads
// game-thread bridge. Rejected requests still receive an id and diagnostic
// snapshot so callers can surface a precise reason.
[[nodiscard]] std::uint64_t RequestRegisteredScriptFunction(
    std::string_view descriptorId,
    std::vector<GTA_Script_Value> arguments = {}) noexcept;

// Restricted surface for menu/Lua callers. Internal descriptors can only be
// invoked from trusted C++ paths even when their ids are known.
[[nodiscard]] std::uint64_t RequestUserFacingScriptFunction(
    std::string_view descriptorId,
    std::vector<GTA_Script_Value> arguments = {}) noexcept;
[[nodiscard]] std::uint64_t RequestVerifiedScriptFunction(
    GTA_Script_Function_Descriptor descriptor,
    std::vector<GTA_Script_Value> arguments = {}) noexcept;

// Compatibility wrapper for the initial zero-argument API.
[[nodiscard]] std::uint64_t RequestVerifiedZeroArgumentScriptFunction(
    GTA_Script_Function_Descriptor descriptor) noexcept;

// Trusted C++ runtime path for code that is already executing inside the
// RunScriptThreads bridge and needs a synchronous result. Do not expose this
// directly to untrusted scripts or free-form menu input.
[[nodiscard]] GTA_Script_Function_Invoke_Snapshot InvokeVerifiedScriptFunctionOnGameThread(
    GTA_Script_Function_Descriptor descriptor,
    const std::vector<GTA_Script_Value>& arguments = {}) noexcept;
[[nodiscard]] GTA_Script_Function_Invoke_Snapshot InvokeRegisteredScriptFunctionOnGameThread(
    std::string_view descriptorId,
    const std::vector<GTA_Script_Value>& arguments = {}) noexcept;

[[nodiscard]] GTA_Script_Function_Invoke_Snapshot ScriptFunctionInvokeSnapshot();
[[nodiscard]] std::optional<GTA_Script_Function_Invoke_Snapshot> ScriptFunctionInvokeSnapshot(
    std::uint64_t requestId);
[[nodiscard]] GTA_Script_Function_Invoker_Metrics ScriptFunctionInvokerMetrics() noexcept;
[[nodiscard]] bool HasPendingScriptFunctionInvocation() noexcept;

// Game-thread consumer. One request is drained per call so bursts cannot
// monopolize a RunScriptThreads pass.
[[nodiscard]] bool TickScriptFunctionInvoker() noexcept;

[[nodiscard]] const char* GTA_Script_Value_Type_Name(GTA_Script_Value_Type type) noexcept;
[[nodiscard]] const char* GTA_Script_Function_Invoke_Status_Name(
    GTA_Script_Function_Invoke_Status status) noexcept;
}
