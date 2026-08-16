#include "Lua_Core_Binding.hpp"

#include "Integrations/GTA5_Enhanced/Runtime/GTA_Script_Function_Invoker.hpp"
#include "Scripting/Lua/Bindings/Lua_Binding_Context.hpp"
#include "Scripting/Lua/Fingerprint/Lua_Fingerprint.hpp"
#include "Scripting/Lua/Lua_Binding_Library.hpp"
#include "Scripting/Lua/Lua_Commands.hpp"
#include "Scripting/Lua/Lua_Engine.hpp"
#include "Scripting/Lua/Lua_Scheduler.hpp"

#include <sol/sol.hpp>

#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace Devilz::Scripting::Lua::Bindings::Core
{
namespace
{
using Integrations::GTA5_Enhanced::FindScriptFunctionDescriptor;
using Integrations::GTA5_Enhanced::GTA_Script_Function_Descriptor;
using Integrations::GTA5_Enhanced::GTA_Script_Function_Exposure;
using Integrations::GTA5_Enhanced::GTA_Script_Function_Invoke_Status;
using Integrations::GTA5_Enhanced::GTA_Script_Function_Invoke_Status_Name;
using Integrations::GTA5_Enhanced::GTA_Script_Value;
using Integrations::GTA5_Enhanced::GTA_Script_Value_Type;
using Integrations::GTA5_Enhanced::GTA_Script_Value_Type_Name;
using Integrations::GTA5_Enhanced::RegisteredScriptFunctions;
using Integrations::GTA5_Enhanced::RequestUserFacingScriptFunction;
using Integrations::GTA5_Enhanced::ScriptFunctionInvokerMetrics;
using Integrations::GTA5_Enhanced::ScriptFunctionInvokerReady;
using Integrations::GTA5_Enhanced::ScriptFunctionInvokeSnapshot;

class Core_Library final : public Lua_Binding_Library
{
public:
    [[nodiscard]] std::string_view Name() const noexcept override
    {
        return "core";
    }

    bool Register(Lua_Engine& engine, const Lua_Binding_Context& context) override
    {
        return RegisterCore(engine, context);
    }
};

[[nodiscard]] bool Finished(GTA_Script_Function_Invoke_Status status) noexcept
{
    return status != GTA_Script_Function_Invoke_Status::Idle &&
           status != GTA_Script_Function_Invoke_Status::Queued &&
           status != GTA_Script_Function_Invoke_Status::Running;
}

[[nodiscard]] std::optional<GTA_Script_Value> LuaValueToScriptValue(
    const sol::object& object,
    GTA_Script_Value_Type type)
{
    switch (type) {
    case GTA_Script_Value_Type::Int32: {
        if (!object.is<lua_Integer>())
            return std::nullopt;
        const auto value = object.as<lua_Integer>();
        if (value < (std::numeric_limits<std::int32_t>::min)() ||
            value > (std::numeric_limits<std::int32_t>::max)())
            return std::nullopt;
        return GTA_Script_Value::Int32(static_cast<std::int32_t>(value));
    }
    case GTA_Script_Value_Type::UInt32:
    case GTA_Script_Value_Type::Hash32: {
        if (!object.is<lua_Integer>())
            return std::nullopt;
        const auto value = object.as<lua_Integer>();
        if (value < 0 ||
            static_cast<std::uint64_t>(value) >
                (std::numeric_limits<std::uint32_t>::max)())
            return std::nullopt;
        const auto packed = static_cast<std::uint32_t>(value);
        return type == GTA_Script_Value_Type::Hash32
            ? GTA_Script_Value::Hash32(packed)
            : GTA_Script_Value::UInt32(packed);
    }
    case GTA_Script_Value_Type::Int64:
        if (!object.is<lua_Integer>())
            return std::nullopt;
        return GTA_Script_Value::Int64(static_cast<std::int64_t>(object.as<lua_Integer>()));
    case GTA_Script_Value_Type::UInt64: {
        // Lua integers are signed. Keep the scripting surface exact rather than
        // accepting imprecise floating-point values for 64-bit VM slots.
        if (!object.is<lua_Integer>())
            return std::nullopt;
        const auto value = object.as<lua_Integer>();
        if (value < 0)
            return std::nullopt;
        return GTA_Script_Value::UInt64(static_cast<std::uint64_t>(value));
    }
    case GTA_Script_Value_Type::Float: {
        if (!object.is<double>() && !object.is<lua_Integer>())
            return std::nullopt;
        const double value = object.as<double>();
        if (!std::isfinite(value) ||
            value < -(std::numeric_limits<float>::max)() ||
            value > (std::numeric_limits<float>::max)())
            return std::nullopt;
        return GTA_Script_Value::Float(static_cast<float>(value));
    }
    case GTA_Script_Value_Type::Bool:
        if (!object.is<bool>())
            return std::nullopt;
        return GTA_Script_Value::Bool(object.as<bool>());
    case GTA_Script_Value_Type::Void:
    default:
        return std::nullopt;
    }
}

[[nodiscard]] std::string AbiText(const GTA_Script_Function_Descriptor& descriptor)
{
    std::string text = "(";
    for (std::size_t index = 0; index < descriptor.abi.arguments.size(); ++index) {
        if (index != 0)
            text += ", ";
        text += GTA_Script_Value_Type_Name(descriptor.abi.arguments[index]);
    }
    text += ") -> ";
    text += GTA_Script_Value_Type_Name(descriptor.abi.returnType);
    return text;
}

void PublishScriptReturn(sol::table& table, const GTA_Script_Value& value)
{
    table["return_type"] = GTA_Script_Value_Type_Name(value.type);
    table["return_raw"] = std::to_string(value.slot);

    switch (value.type) {
    case GTA_Script_Value_Type::Int32:
        if (const auto typed = value.AsInt32())
            table["return_value"] = *typed;
        break;
    case GTA_Script_Value_Type::UInt32:
        if (const auto typed = value.AsUInt32())
            table["return_value"] = static_cast<lua_Integer>(*typed);
        break;
    case GTA_Script_Value_Type::Int64:
        if (const auto typed = value.AsInt64())
            table["return_value"] = static_cast<lua_Integer>(*typed);
        break;
    case GTA_Script_Value_Type::UInt64:
        if (const auto typed = value.AsUInt64()) {
            if (*typed <= static_cast<std::uint64_t>((std::numeric_limits<lua_Integer>::max)()))
                table["return_value"] = static_cast<lua_Integer>(*typed);
        }
        break;
    case GTA_Script_Value_Type::Float:
        if (const auto typed = value.AsFloat())
            table["return_value"] = static_cast<double>(*typed);
        break;
    case GTA_Script_Value_Type::Bool:
        if (const auto typed = value.AsBool())
            table["return_value"] = *typed;
        break;
    case GTA_Script_Value_Type::Hash32:
        if (const auto typed = value.AsHash32())
            table["return_value"] = static_cast<lua_Integer>(*typed);
        break;
    case GTA_Script_Value_Type::Void:
    default:
        break;
    }
}

sol::table ScriptFunctionList(sol::state_view state)
{
    auto result = state.create_table();
    const auto descriptors = RegisteredScriptFunctions(true);
    std::size_t outputIndex = 1;
    for (const auto& descriptor : descriptors) {
        auto item = state.create_table();
        item["id"] = descriptor.id;
        item["label"] = descriptor.label;
        item["category"] = descriptor.category;
        item["script"] = descriptor.scriptName;
        item["abi"] = AbiText(descriptor);
        item["source"] = !descriptor.source.path.empty()
            ? descriptor.source.path
            : descriptor.sourceReference;
        item["source_function"] = descriptor.source.function;
        item["source_revision"] = descriptor.source.revision;

        auto argumentTypes = state.create_table();
        for (std::size_t argument = 0; argument < descriptor.abi.arguments.size(); ++argument)
            argumentTypes[argument + 1] = GTA_Script_Value_Type_Name(descriptor.abi.arguments[argument]);
        item["argument_types"] = argumentTypes;
        item["return_type"] = GTA_Script_Value_Type_Name(descriptor.abi.returnType);
        result[outputIndex++] = item;
    }
    return result;
}

std::tuple<std::uint64_t, std::string> QueueScriptFunction(
    const std::string& descriptorId,
    sol::optional<sol::table> luaArguments)
{
    const auto descriptor = FindScriptFunctionDescriptor(descriptorId);
    if (!descriptor)
        return {0, "descriptor not found"};
    if (descriptor->exposure != GTA_Script_Function_Exposure::UserFacing)
        return {0, "descriptor is internal-only"};

    const std::size_t provided = luaArguments ? luaArguments->size() : 0;
    if (provided != descriptor->abi.arguments.size())
        return {0, "expected " + std::to_string(descriptor->abi.arguments.size()) +
                   " arguments, got " + std::to_string(provided)};

    std::vector<GTA_Script_Value> arguments;
    arguments.reserve(descriptor->abi.arguments.size());
    if (luaArguments) {
        for (std::size_t index = 0; index < descriptor->abi.arguments.size(); ++index) {
            const sol::object value = luaArguments->get<sol::object>(index + 1);
            const auto packed = LuaValueToScriptValue(value, descriptor->abi.arguments[index]);
            if (!packed) {
                return {0, "argument " + std::to_string(index + 1) +
                           " must be " + GTA_Script_Value_Type_Name(descriptor->abi.arguments[index])};
            }
            arguments.push_back(*packed);
        }
    }

    const auto requestId = RequestUserFacingScriptFunction(descriptorId, std::move(arguments));
    if (requestId == 0)
        return {0, "script VM request could not be created"};

    const auto snapshot = ScriptFunctionInvokeSnapshot(requestId);
    if (snapshot && Finished(snapshot->status) &&
        snapshot->status != GTA_Script_Function_Invoke_Status::Executed) {
        return {requestId, snapshot->detail};
    }
    return {requestId, "queued"};
}

sol::table ScriptFunctionStatus(sol::state_view state, std::uint64_t requestId)
{
    auto result = state.create_table();
    const auto snapshot = ScriptFunctionInvokeSnapshot(requestId);
    result["found"] = snapshot.has_value();
    if (!snapshot)
        return result;

    result["request_id"] = snapshot->requestId;
    result["descriptor_id"] = snapshot->descriptorId;
    result["label"] = snapshot->label;
    result["script"] = snapshot->scriptName;
    result["status"] = GTA_Script_Function_Invoke_Status_Name(snapshot->status);
    result["done"] = Finished(snapshot->status);
    result["success"] = snapshot->status == GTA_Script_Function_Invoke_Status::Executed;
    result["detail"] = snapshot->detail;
    result["signature_pc"] = snapshot->signatureProgramCounter;
    result["entry_pc"] = snapshot->resolvedProgramCounter;
    result["cache_hit"] = snapshot->cacheHit;
    result["vm_result"] = snapshot->scriptVmResult;
    if (snapshot->returnValue)
        PublishScriptReturn(result, *snapshot->returnValue);
    return result;
}

sol::table ScriptFunctionMetrics(sol::state_view state)
{
    const auto metrics = ScriptFunctionInvokerMetrics();
    auto result = state.create_table();
    result["ready"] = ScriptFunctionInvokerReady();
    result["registered"] = metrics.registeredDescriptors;
    result["pending"] = metrics.pendingRequests;
    result["cached"] = metrics.cachedDescriptors;
    result["queued"] = metrics.requestsQueued;
    result["rejected"] = metrics.requestsRejected;
    result["executed"] = metrics.invocationsExecuted;
    result["failed"] = metrics.invocationsFailed;
    result["cache_hits"] = metrics.cacheHits;
    result["cache_misses"] = metrics.cacheMisses;
    result["pattern_scans"] = metrics.patternScans;
    result["signature_revalidations"] = metrics.signatureRevalidations;
    return result;
}
}

bool RegisterCore(Lua_Engine& engine, const Lua_Binding_Context& context)
{
    if (!engine.Ready() || !context.commands || !context.fingerprints)
        return false;

    const auto& runtimeFingerprint = context.fingerprints->Runtime();
    if (runtimeFingerprint.value == 0)
        return false;

    auto& state = engine.State();
    auto devilz = state.create_named_table("devilz");
    const auto fingerprintHex = Lua_Fingerprint_Manager::ToHex(runtimeFingerprint.value);

    devilz["api_version"] = runtimeFingerprint.apiVersion;
    devilz["runtime"] = "Devilz Den";
    devilz["engine_id"] = engine.GetId();
    devilz["owner_script_id"] = engine.OwnerScriptId();
    devilz["fingerprint"] = fingerprintHex;
    devilz["runtime_fingerprint"] = fingerprintHex;

    auto runtimeInfo = state.create_table();
    runtimeInfo["name"] = "Devilz Den";
    runtimeInfo["api_version"] = runtimeFingerprint.apiVersion;
    runtimeInfo["fingerprint"] = fingerprintHex;
    runtimeInfo["lua_version"] = runtimeFingerprint.luaVersion;
    runtimeInfo["sol2_version"] = runtimeFingerprint.sol2Version;
    devilz["runtime_info"] = runtimeInfo;

    devilz.set_function("version", []() {
        return std::string{LUA_RELEASE};
    });

    auto commandTable = state.create_table();
    const auto ownerScriptId = engine.OwnerScriptId();
    auto* commands = context.commands;

    commandTable.set_function(
        "register",
        [commands, ownerScriptId](std::string name, sol::protected_function callback) {
            return commands->Register(ownerScriptId, std::move(name), {}, std::move(callback));
        });

    commandTable.set_function(
        "execute",
        [commands](const std::string& name) {
            auto result = commands->Execute(name);
            return std::make_tuple(result.succeeded, std::move(result.message));
        });

    devilz["commands"] = commandTable;

    // Safe ScriptVM surface: Lua can discover and queue only descriptors that
    // trusted C++ has explicitly registered as UserFacing. It cannot supply a
    // raw PC, byte signature, script hash, or build override.
    auto scriptVm = state.create_table();
    scriptVm.set_function("available", []() { return ScriptFunctionInvokerReady(); });
    scriptVm.set_function("list", [&state]() { return ScriptFunctionList(state); });
    scriptVm.set_function(
        "invoke",
        [](const std::string& descriptorId, sol::optional<sol::table> arguments) {
            return QueueScriptFunction(descriptorId, std::move(arguments));
        });
    scriptVm.set_function(
        "status",
        [&state](std::uint64_t requestId) { return ScriptFunctionStatus(state, requestId); });
    scriptVm.set_function("metrics", [&state]() { return ScriptFunctionMetrics(state); });
    devilz["script_vm"] = scriptVm;

    return Lua_Scheduler::Install(engine);
}

std::unique_ptr<Lua_Binding_Library> CreateCoreLibrary()
{
    return std::make_unique<Core_Library>();
}
}
