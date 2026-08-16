void ClearScriptFunctionPcCache() noexcept
{
    try {
        std::scoped_lock lock(g_cacheMutex);
        g_pcCache.clear();
    } catch (...) {
    }
}

std::uint64_t RequestRegisteredScriptFunction(
    std::string_view descriptorId,
    std::vector<GTA_Script_Value> arguments) noexcept
{
    try {
        std::scoped_lock lock(g_stateMutex);
        const auto it = g_registry.find(std::string(descriptorId));
        if (it == g_registry.end()) {
            const auto requestId = NextRequestIdLocked();
            g_requestsRejected.fetch_add(1, std::memory_order_relaxed);
            auto snapshot = MakeRejectedSnapshot(
                requestId,
                nullptr,
                GTA_Script_Function_Invoke_Status::DescriptorNotFound,
                "No verified script function descriptor is registered with id '" +
                    std::string(descriptorId) + "'");
            snapshot.descriptorId = std::string(descriptorId);
            StoreSnapshotLocked(std::move(snapshot));
            return requestId;
        }
        return QueueRequestLocked(it->second, std::move(arguments));
    } catch (...) {
        return 0;
    }
}

std::uint64_t RequestUserFacingScriptFunction(
    std::string_view descriptorId,
    std::vector<GTA_Script_Value> arguments) noexcept
{
    try {
        std::scoped_lock lock(g_stateMutex);
        const auto it = g_registry.find(std::string(descriptorId));
        if (it == g_registry.end()) {
            const auto requestId = NextRequestIdLocked();
            g_requestsRejected.fetch_add(1, std::memory_order_relaxed);
            auto snapshot = MakeRejectedSnapshot(
                requestId,
                nullptr,
                GTA_Script_Function_Invoke_Status::DescriptorNotFound,
                "No verified script function descriptor is registered with id '" +
                    std::string(descriptorId) + "'");
            snapshot.descriptorId = std::string(descriptorId);
            StoreSnapshotLocked(std::move(snapshot));
            return requestId;
        }
        if (it->second.exposure != GTA_Script_Function_Exposure::UserFacing) {
            const auto requestId = NextRequestIdLocked();
            g_requestsRejected.fetch_add(1, std::memory_order_relaxed);
            StoreSnapshotLocked(MakeRejectedSnapshot(
                requestId,
                &it->second,
                GTA_Script_Function_Invoke_Status::DescriptorNotUserFacing,
                "Registered descriptor is internal-only and cannot be invoked from menu/Lua surfaces"));
            return requestId;
        }
        return QueueRequestLocked(it->second, std::move(arguments));
    } catch (...) {
        return 0;
    }
}

std::uint64_t RequestVerifiedScriptFunction(
    GTA_Script_Function_Descriptor descriptor,
    std::vector<GTA_Script_Value> arguments) noexcept
{
    try {
        std::scoped_lock lock(g_stateMutex);
        return QueueRequestLocked(std::move(descriptor), std::move(arguments));
    } catch (...) {
        return 0;
    }
}

std::uint64_t RequestVerifiedZeroArgumentScriptFunction(
    GTA_Script_Function_Descriptor descriptor) noexcept
{
    const auto requestId = RequestVerifiedScriptFunction(std::move(descriptor), {});
    if (requestId == 0)
        return 0;
    try {
        const auto snapshot = ScriptFunctionInvokeSnapshot(requestId);
        if (snapshot && snapshot->status == GTA_Script_Function_Invoke_Status::QueueFull)
            return 0;
    } catch (...) {
    }
    return requestId;
}

GTA_Script_Function_Invoke_Snapshot InvokeVerifiedScriptFunctionOnGameThread(
    GTA_Script_Function_Descriptor descriptor,
    const std::vector<GTA_Script_Value>& arguments) noexcept
{
    return RunImmediate(std::move(descriptor), arguments);
}

GTA_Script_Function_Invoke_Snapshot InvokeRegisteredScriptFunctionOnGameThread(
    std::string_view descriptorId,
    const std::vector<GTA_Script_Value>& arguments) noexcept
{
    GTA_Script_Function_Descriptor descriptor;
    try {
        {
            std::scoped_lock lock(g_stateMutex);
            const auto it = g_registry.find(std::string(descriptorId));
            if (it == g_registry.end()) {
                const auto requestId = NextRequestIdLocked();
                g_requestsRejected.fetch_add(1, std::memory_order_relaxed);
                auto snapshot = MakeRejectedSnapshot(
                    requestId,
                    nullptr,
                    GTA_Script_Function_Invoke_Status::DescriptorNotFound,
                    "No verified script function descriptor is registered with id '" +
                        std::string(descriptorId) + "'");
                snapshot.descriptorId = std::string(descriptorId);
                StoreSnapshotLocked(snapshot);
                return snapshot;
            }
            descriptor = it->second;
        }
    } catch (...) {
        GTA_Script_Function_Invoke_Snapshot failed{};
        failed.status = GTA_Script_Function_Invoke_Status::Failed;
        failed.detail = "Unable to resolve registered script function descriptor";
        return failed;
    }
    return RunImmediate(std::move(descriptor), arguments);
}

GTA_Script_Function_Invoke_Snapshot ScriptFunctionInvokeSnapshot()
{
    std::scoped_lock lock(g_stateMutex);
    return g_latestSnapshot;
}

std::optional<GTA_Script_Function_Invoke_Snapshot> ScriptFunctionInvokeSnapshot(
    std::uint64_t requestId)
{
    std::scoped_lock lock(g_stateMutex);
    const auto it = g_history.find(requestId);
    if (it == g_history.end())
        return std::nullopt;
    return it->second;
}

GTA_Script_Function_Invoker_Metrics ScriptFunctionInvokerMetrics() noexcept
{
    GTA_Script_Function_Invoker_Metrics metrics{};
    metrics.requestsQueued = g_requestsQueued.load(std::memory_order_relaxed);
    metrics.requestsRejected = g_requestsRejected.load(std::memory_order_relaxed);
    metrics.invocationsExecuted = g_invocationsExecuted.load(std::memory_order_relaxed);
    metrics.invocationsFailed = g_invocationsFailed.load(std::memory_order_relaxed);
    metrics.cacheHits = g_cacheHits.load(std::memory_order_relaxed);
    metrics.cacheMisses = g_cacheMisses.load(std::memory_order_relaxed);
    metrics.signatureRevalidations = g_signatureRevalidations.load(std::memory_order_relaxed);
    metrics.patternScans = g_patternScans.load(std::memory_order_relaxed);
    metrics.pendingRequests = g_pendingCount.load(std::memory_order_acquire);

    try {
        {
            std::scoped_lock lock(g_stateMutex);
            metrics.registeredDescriptors = g_registry.size();
        }
        {
            std::scoped_lock lock(g_cacheMutex);
            metrics.cachedDescriptors = g_pcCache.size();
        }
    } catch (...) {
    }
    return metrics;
}

bool HasPendingScriptFunctionInvocation() noexcept
{
    return g_pendingCount.load(std::memory_order_acquire) != 0;
}

bool TickScriptFunctionInvoker() noexcept
{
    if (!HasPendingScriptFunctionInvocation())
        return false;

    ScriptFunctionRuntime runtime{};
    std::optional<ScriptFunctionRequest> request;
    try {
        std::scoped_lock lock(g_stateMutex);
        if (g_queue.empty()) {
            g_pendingCount.store(0, std::memory_order_release);
            return false;
        }

        request = std::move(g_queue.front());
        g_queue.pop_front();
        g_pendingCount.store(g_queue.size(), std::memory_order_release);
        runtime = g_runtime;

        GTA_Script_Function_Invoke_Snapshot running{};
        running.requestId = request->requestId;
        running.status = GTA_Script_Function_Invoke_Status::Running;
        running.descriptorId = request->descriptor.id;
        running.label = request->descriptor.label;
        running.scriptName = request->descriptor.scriptName;
        running.detail = "Executing through Rockstar ScriptVM on the GTA game thread";
        StoreSnapshotLocked(std::move(running));
    } catch (...) {
        return false;
    }
