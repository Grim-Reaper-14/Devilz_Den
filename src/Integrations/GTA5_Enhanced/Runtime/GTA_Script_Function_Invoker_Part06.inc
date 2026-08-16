        g_invocationsExecuted.fetch_add(1, std::memory_order_relaxed);
    else
        g_invocationsFailed.fetch_add(1, std::memory_order_relaxed);
}

std::uint64_t NextRequestIdLocked() noexcept
{
    const auto id = g_nextRequestId++;
    if (g_nextRequestId == 0)
        g_nextRequestId = 1;
    return id == 0 ? NextRequestIdLocked() : id;
}

void StoreSnapshotLocked(GTA_Script_Function_Invoke_Snapshot snapshot)
{
    const auto requestId = snapshot.requestId;
    if (requestId == 0)
        return;

    if (!g_history.contains(requestId))
        g_historyOrder.push_back(requestId);
    g_history.insert_or_assign(requestId, snapshot);

    if (requestId >= g_latestRequestId) {
        g_latestRequestId = requestId;
        g_latestSnapshot = std::move(snapshot);
    } else if (requestId == g_latestSnapshot.requestId) {
        g_latestSnapshot = std::move(snapshot);
    }

    while (g_historyOrder.size() > MaxRequestHistory) {
        const auto oldest = g_historyOrder.front();
        g_historyOrder.pop_front();
        g_history.erase(oldest);
    }
}

GTA_Script_Function_Invoke_Snapshot MakeRejectedSnapshot(
    std::uint64_t requestId,
    const GTA_Script_Function_Descriptor* descriptor,
    GTA_Script_Function_Invoke_Status status,
    std::string detail)
{
    GTA_Script_Function_Invoke_Snapshot snapshot{};
    snapshot.requestId = requestId;
    snapshot.status = status;
    if (descriptor) {
        snapshot.descriptorId = descriptor->id;
        snapshot.label = descriptor->label;
        snapshot.scriptName = descriptor->scriptName;
    }
    snapshot.detail = std::move(detail);
    return snapshot;
}

std::uint64_t QueueRequestLocked(
    GTA_Script_Function_Descriptor descriptor,
    std::vector<GTA_Script_Value> arguments)
{
    EnsureDescriptorId(descriptor);
    const auto requestId = NextRequestIdLocked();

    auto reject = [&](GTA_Script_Function_Invoke_Status status, std::string detail) {
        g_requestsRejected.fetch_add(1, std::memory_order_relaxed);
        StoreSnapshotLocked(MakeRejectedSnapshot(requestId, &descriptor, status, std::move(detail)));
        return requestId;
    };

    if (!g_runtime.Ready())
        return reject(GTA_Script_Function_Invoke_Status::RuntimeUnavailable,
                      "Script function runtime is not configured");
    if (!DescriptorValid(descriptor))
        return reject(GTA_Script_Function_Invoke_Status::InvalidDescriptor,
                      "Descriptor is missing verified source/build/signature/ABI metadata");
    if (descriptor.expectedBuildFingerprint != g_runtime.buildFingerprint)
        return reject(GTA_Script_Function_Invoke_Status::UnsupportedBuild,
                      "Descriptor does not target the running GTA build");

    const auto argumentError = ValidateArguments(descriptor, arguments);
    if (argumentError) {
        if (*argumentError == std::numeric_limits<std::size_t>::max()) {
            return reject(GTA_Script_Function_Invoke_Status::ArgumentCountMismatch,
                          "Argument count does not match the registered Rockstar script ABI");
        }
        return reject(GTA_Script_Function_Invoke_Status::ArgumentTypeMismatch,
                      "Argument type mismatch at index " + std::to_string(*argumentError));
    }

    if (g_queue.size() >= MaxPendingRequests)
        return reject(GTA_Script_Function_Invoke_Status::QueueFull,
                      "Script function request queue is full; existing work was preserved");

    GTA_Script_Function_Invoke_Snapshot snapshot{};
    snapshot.requestId = requestId;
    snapshot.status = GTA_Script_Function_Invoke_Status::Queued;
    snapshot.descriptorId = descriptor.id;
    snapshot.label = descriptor.label;
    snapshot.scriptName = descriptor.scriptName;
    snapshot.detail = "Queued for the existing RunScriptThreads game-thread bridge";
    StoreSnapshotLocked(snapshot);

    g_queue.push_back(ScriptFunctionRequest{
        requestId,
        std::move(descriptor),
        std::move(arguments),
    });
    g_pendingCount.store(g_queue.size(), std::memory_order_release);
    g_requestsQueued.fetch_add(1, std::memory_order_relaxed);
    return requestId;
}

GTA_Script_Function_Invoke_Snapshot RunImmediate(
    GTA_Script_Function_Descriptor descriptor,
    const std::vector<GTA_Script_Value>& arguments) noexcept
{
    try {
        EnsureDescriptorId(descriptor);
    } catch (...) {
        GTA_Script_Function_Invoke_Snapshot failed{};
        failed.status = GTA_Script_Function_Invoke_Status::Failed;
        failed.detail = "Unable to construct a stable script function descriptor id";
        return failed;
    }

    ScriptFunctionRuntime runtime{};
    ScriptFunctionRequest request{};
    try {
        std::scoped_lock lock(g_stateMutex);
        request.requestId = NextRequestIdLocked();
        request.descriptor = std::move(descriptor);
        request.arguments = arguments;
        runtime = g_runtime;

        GTA_Script_Function_Invoke_Snapshot running{};
        running.requestId = request.requestId;
        running.status = GTA_Script_Function_Invoke_Status::Running;
        running.descriptorId = request.descriptor.id;
        running.label = request.descriptor.label;
        running.scriptName = request.descriptor.scriptName;
        running.detail = "Executing synchronously through Rockstar ScriptVM on the GTA game thread";
        StoreSnapshotLocked(running);
    } catch (...) {
        GTA_Script_Function_Invoke_Snapshot failed{};
        failed.status = GTA_Script_Function_Invoke_Status::Failed;
        failed.detail = "Unable to prepare synchronous script function request";
        return failed;
    }

    auto result = Execute(runtime, request);
    CountCompletion(result);
    LogCompletion(runtime, result);
    try {
        std::scoped_lock lock(g_stateMutex);
        StoreSnapshotLocked(result);
    } catch (...) {
    }
    return result;
}
}

GTA_Script_Value GTA_Script_Value::Int32(std::int32_t value) noexcept
{
    return {GTA_Script_Value_Type::Int32,
            static_cast<std::uint64_t>(std::bit_cast<std::uint32_t>(value))};
}

GTA_Script_Value GTA_Script_Value::UInt32(std::uint32_t value) noexcept
{
    return {GTA_Script_Value_Type::UInt32, value};
}

GTA_Script_Value GTA_Script_Value::Int64(std::int64_t value) noexcept
{
    return {GTA_Script_Value_Type::Int64, std::bit_cast<std::uint64_t>(value)};
}

GTA_Script_Value GTA_Script_Value::UInt64(std::uint64_t value) noexcept
{
    return {GTA_Script_Value_Type::UInt64, value};
}

GTA_Script_Value GTA_Script_Value::Float(float value) noexcept
{
    return {GTA_Script_Value_Type::Float,
            static_cast<std::uint64_t>(std::bit_cast<std::uint32_t>(value))};
}

GTA_Script_Value GTA_Script_Value::Bool(bool value) noexcept
{
    return {GTA_Script_Value_Type::Bool, value ? 1ULL : 0ULL};
}

GTA_Script_Value GTA_Script_Value::Hash32(std::uint32_t value) noexcept
{
    return {GTA_Script_Value_Type::Hash32, value};
}

std::optional<std::int32_t> GTA_Script_Value::AsInt32() const noexcept
{
    if (type != GTA_Script_Value_Type::Int32)
        return std::nullopt;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(slot));
}

std::optional<std::uint32_t> GTA_Script_Value::AsUInt32() const noexcept
{
    if (type != GTA_Script_Value_Type::UInt32)
        return std::nullopt;
    return static_cast<std::uint32_t>(slot);
}

std::optional<std::int64_t> GTA_Script_Value::AsInt64() const noexcept
{
    if (type != GTA_Script_Value_Type::Int64)
        return std::nullopt;
    return std::bit_cast<std::int64_t>(slot);
}

std::optional<std::uint64_t> GTA_Script_Value::AsUInt64() const noexcept
{
    if (type != GTA_Script_Value_Type::UInt64)
        return std::nullopt;
    return slot;
}

std::optional<float> GTA_Script_Value::AsFloat() const noexcept
{
    if (type != GTA_Script_Value_Type::Float)
        return std::nullopt;
    return std::bit_cast<float>(static_cast<std::uint32_t>(slot));
}

