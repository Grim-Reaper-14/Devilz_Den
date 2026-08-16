std::optional<bool> GTA_Script_Value::AsBool() const noexcept
{
    if (type != GTA_Script_Value_Type::Bool)
        return std::nullopt;
    return (slot & 0xFFFFFFFFULL) != 0;
}

std::optional<std::uint32_t> GTA_Script_Value::AsHash32() const noexcept
{
    if (type != GTA_Script_Value_Type::Hash32)
        return std::nullopt;
    return static_cast<std::uint32_t>(slot);
}

const char* GTA_Script_Value_Type_Name(GTA_Script_Value_Type type) noexcept
{
    switch (type) {
    case GTA_Script_Value_Type::Void: return "VOID";
    case GTA_Script_Value_Type::Int32: return "INT32";
    case GTA_Script_Value_Type::UInt32: return "UINT32";
    case GTA_Script_Value_Type::Int64: return "INT64";
    case GTA_Script_Value_Type::UInt64: return "UINT64";
    case GTA_Script_Value_Type::Float: return "FLOAT";
    case GTA_Script_Value_Type::Bool: return "BOOL";
    case GTA_Script_Value_Type::Hash32: return "HASH32";
    default: return "UNKNOWN";
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
    case GTA_Script_Function_Invoke_Status::DescriptorNotFound: return "DESCRIPTOR NOT FOUND";
    case GTA_Script_Function_Invoke_Status::DescriptorNotUserFacing: return "DESCRIPTOR NOT USER FACING";
    case GTA_Script_Function_Invoke_Status::QueueFull: return "QUEUE FULL";
    case GTA_Script_Function_Invoke_Status::ArgumentCountMismatch: return "ARGUMENT COUNT MISMATCH";
    case GTA_Script_Function_Invoke_Status::ArgumentTypeMismatch: return "ARGUMENT TYPE MISMATCH";
    case GTA_Script_Function_Invoke_Status::ScriptProgramNotFound: return "SCRIPT PROGRAM NOT FOUND";
    case GTA_Script_Function_Invoke_Status::ScriptThreadNotFound: return "SCRIPT THREAD NOT FOUND";
    case GTA_Script_Function_Invoke_Status::ScriptCodeSizeMismatch: return "SCRIPT CODE SIZE MISMATCH";
    case GTA_Script_Function_Invoke_Status::ProgramCounterOutOfRange: return "PROGRAM COUNTER OUT OF RANGE";
    case GTA_Script_Function_Invoke_Status::SignatureMismatch: return "SIGNATURE MISMATCH";
    case GTA_Script_Function_Invoke_Status::PatternNotFound: return "PATTERN NOT FOUND";
    case GTA_Script_Function_Invoke_Status::PatternNotUnique: return "PATTERN NOT UNIQUE";
    case GTA_Script_Function_Invoke_Status::StackUnavailable: return "STACK UNAVAILABLE";
    case GTA_Script_Function_Invoke_Status::NotOnGameThread: return "NOT ON GAME THREAD";
    case GTA_Script_Function_Invoke_Status::ScriptVmUnavailable: return "SCRIPT VM UNAVAILABLE";
    case GTA_Script_Function_Invoke_Status::UnexpectedYield: return "UNEXPECTED YIELD";
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
    try {
        {
            std::scoped_lock lock(g_stateMutex);
            g_runtime = {
                scriptGlobalsAddress,
                programTableAddress,
                scriptThreadsStorageAddress,
                scriptVmAddress,
                buildFingerprint,
                logger,
            };
            g_queue.clear();
            g_history.clear();
            g_historyOrder.clear();
            g_latestSnapshot = {};
            g_latestRequestId = 0;
            g_pendingCount.store(0, std::memory_order_release);
            g_nextRequestId = std::max<std::uint64_t>(g_nextRequestId, 1);
        }
        ClearScriptFunctionPcCache();
        ResetMetrics();

        if (logger) {
            const bool ready = ScriptFunctionInvokerReady();
            logger->Log(
                ready ? Backend::LogLevel::Info : Backend::LogLevel::Warning,
                ready
                    ? "Verified Rockstar ScriptVM function invoker ready | typed ABI + signature-revalidated PC cache + bounded queue"
                    : "Script function invoker unavailable: ScriptGlobals, ProgramTable, ScriptThreads, or ScriptVM target missing",
                "GTA5_Enhanced.ScriptFunction");
        }
    } catch (...) {
        g_pendingCount.store(0, std::memory_order_release);
    }
}

void ResetScriptFunctionInvoker() noexcept
{
    try {
        {
            std::scoped_lock lock(g_stateMutex);
            g_runtime = {};
            g_queue.clear();
            g_history.clear();
            g_historyOrder.clear();
            g_latestSnapshot = {};
            g_latestRequestId = 0;
            g_pendingCount.store(0, std::memory_order_release);
            g_nextRequestId = std::max<std::uint64_t>(g_nextRequestId, 1);
        }
        ClearScriptFunctionPcCache();
        ResetMetrics();
    } catch (...) {
        g_pendingCount.store(0, std::memory_order_release);
    }
}

bool ScriptFunctionInvokerReady() noexcept
{
    try {
        std::scoped_lock lock(g_stateMutex);
        return g_runtime.Ready();
    } catch (...) {
        return false;
    }
}

bool RegisterScriptFunctionDescriptor(GTA_Script_Function_Descriptor descriptor) noexcept
{
    try {
        EnsureDescriptorId(descriptor);
    } catch (...) {
        return false;
    }
    if (!DescriptorValid(descriptor))
        return false;

    try {
        const auto id = descriptor.id;
        {
            std::scoped_lock lock(g_stateMutex);
            g_registry.insert_or_assign(id, std::move(descriptor));
        }
        InvalidateCacheEntry(id);
        return true;
    } catch (...) {
        return false;
    }
}

bool UnregisterScriptFunctionDescriptor(std::string_view descriptorId) noexcept
{
    if (descriptorId.empty())
        return false;
    try {
        bool erased = false;
        {
            std::scoped_lock lock(g_stateMutex);
            erased = g_registry.erase(std::string(descriptorId)) != 0;
        }
        InvalidateCacheEntry(descriptorId);
        return erased;
    } catch (...) {
        return false;
    }
}

std::optional<GTA_Script_Function_Descriptor> FindScriptFunctionDescriptor(
    std::string_view descriptorId)
{
    std::scoped_lock lock(g_stateMutex);
    const auto it = g_registry.find(std::string(descriptorId));
    if (it == g_registry.end())
        return std::nullopt;
    return it->second;
}

std::vector<GTA_Script_Function_Descriptor> RegisteredScriptFunctions(bool userFacingOnly)
{
    std::vector<GTA_Script_Function_Descriptor> descriptors;
    {
        std::scoped_lock lock(g_stateMutex);
        descriptors.reserve(g_registry.size());
        for (const auto& [id, descriptor] : g_registry) {
            (void)id;
            if (userFacingOnly &&
                descriptor.exposure != GTA_Script_Function_Exposure::UserFacing)
                continue;
            descriptors.push_back(descriptor);
        }
    }
    std::sort(descriptors.begin(), descriptors.end(), [](const auto& left, const auto& right) {
        if (left.category != right.category)
            return left.category < right.category;
        if (left.label != right.label)
            return left.label < right.label;
        return left.id < right.id;
    });
    return descriptors;
}

void ClearScriptFunctionRegistry() noexcept
{
    try {
        {
            std::scoped_lock lock(g_stateMutex);
            g_registry.clear();
        }
        ClearScriptFunctionPcCache();
    } catch (...) {
    }
}

