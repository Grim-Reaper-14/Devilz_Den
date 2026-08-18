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
        result.detail = "Descriptor is missing verified source/build/signature/ABI metadata";
        return result;
    }

    const auto argumentError = ValidateArguments(descriptor, request.arguments);
    if (argumentError) {
        if (*argumentError == std::numeric_limits<std::size_t>::max()) {
            result.status = GTA_Script_Function_Invoke_Status::ArgumentCountMismatch;
            result.detail = "Argument count does not match the registered Rockstar script ABI";
        } else {
            result.status = GTA_Script_Function_Invoke_Status::ArgumentTypeMismatch;
            result.detail = "Argument type mismatch at index " + std::to_string(*argumentError);
        }
        return result;
    }

    const auto scriptHash = Joaat(descriptor.scriptName);
    auto* program = FindScriptProgram(runtime, scriptHash);
    if (!program) {
        result.status = GTA_Script_Function_Invoke_Status::ScriptProgramNotFound;
        result.detail = "Target Rockstar script program is not loaded";
        return result;
    }
    if (descriptor.expectedScriptCodeSize != 0 &&
        descriptor.expectedScriptCodeSize != program->codeSize) {
        result.status = GTA_Script_Function_Invoke_Status::ScriptCodeSizeMismatch;
        result.detail = "Loaded script code size does not match the verified descriptor";
        return result;
    }

    ResolvedFunction resolved{};
    if (!ResolveProgramCounter(runtime, descriptor, program, resolved, result))
        return result;
    result.signatureProgramCounter = resolved.signatureProgramCounter;
    result.resolvedProgramCounter = resolved.resolvedProgramCounter;
    result.cacheHit = resolved.cacheHit;

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
    const std::size_t requiredSlots = request.arguments.size() + 1;
    if (tailSlots < requiredSlots ||
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
    if (!tls->scriptThreadActive || tls->currentScriptThread == nullptr) {
        result.status = GTA_Script_Function_Invoke_Status::NotOnGameThread;
        result.detail = "Invocation was rejected outside an active GTA script-thread context";
        return result;
    }

    // Run on Rockstar's live stack for maximum VM/native compatibility, but
    // preserve the complete unused tail so the host script resumes byte-for-byte
    // after the verified synchronous helper returns.
    thread_local std::vector<std::uint64_t> savedStackTail;
    try {
        savedStackTail.resize(tailSlots);
    } catch (...) {
        result.status = GTA_Script_Function_Invoke_Status::StackUnavailable;
        result.detail = "Unable to allocate temporary storage for script stack preservation";
        return result;
    }
    std::memcpy(savedStackTail.data(), thread->stack + stackPointer,
                tailSlots * sizeof(std::uint64_t));

    auto restoreStack = [&]() noexcept {
        std::memcpy(thread->stack + stackPointer, savedStackTail.data(),
                    tailSlots * sizeof(std::uint64_t));
    };

    auto context = thread->context;
    const auto returnSlot = context.stackPointer;
    for (const auto& argument : request.arguments)
        thread->stack[context.stackPointer++] = argument.slot;
    thread->stack[context.stackPointer++] = 0; // synthetic return PC
    context.programCounter = resolved.resolvedProgramCounter;
    context.state = 0;

    void* previousThread = tls->currentScriptThread;
    const bool previousActive = tls->scriptThreadActive;
    tls->currentScriptThread = thread;
    tls->scriptThreadActive = true;

    try {
        result.scriptVmResult = scriptVm(
            thread->stack,
            reinterpret_cast<std::int64_t**>(runtime.scriptGlobalsAddress),
            program,
            &context);
    } catch (...) {
        tls->scriptThreadActive = previousActive;
        tls->currentScriptThread = previousThread;
        restoreStack();
        result.status = GTA_Script_Function_Invoke_Status::Failed;
        result.detail = "Rockstar ScriptVM invocation raised a C++ exception";
        return result;
    }

    if (descriptor.abi.returnType != GTA_Script_Value_Type::Void)
        result.returnValue = NormalizeReturnValue(descriptor.abi.returnType,
                                                  thread->stack[returnSlot]);

    const auto postState = context.state;
    tls->scriptThreadActive = previousActive;
    tls->currentScriptThread = previousThread;
    restoreStack();

    if (postState != 0 && !descriptor.allowNonIdleCompletion) {
        result.status = GTA_Script_Function_Invoke_Status::UnexpectedYield;
        result.detail = "Verified synchronous function returned with a non-idle script state; stack was restored";
        return result;
    }

    result.status = GTA_Script_Function_Invoke_Status::Executed;
    result.detail = "Verified Rockstar script function executed at PC " +
                    HexPc(resolved.resolvedProgramCounter) +
                    (resolved.cacheHit ? " using the revalidated PC cache" :
                                         "; resolved PC cached for this loaded program") +
                    "; temporary stack changes restored";
    if (postState != 0)
        result.detail += "; verified descriptor accepted the non-idle completion boundary";
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

void CountCompletion(const GTA_Script_Function_Invoke_Snapshot& result) noexcept
{
    if (result.status == GTA_Script_Function_Invoke_Status::Executed)
