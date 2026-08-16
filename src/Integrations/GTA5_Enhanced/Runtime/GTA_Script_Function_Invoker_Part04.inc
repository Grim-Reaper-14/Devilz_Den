    if (!program || cached.descriptorKey != DescriptorKey(descriptor) ||
        cached.buildFingerprint != descriptor.expectedBuildFingerprint ||
        cached.scriptHash != Joaat(descriptor.scriptName) ||
        cached.programAddress != reinterpret_cast<std::uintptr_t>(program) ||
        cached.codeBlocksAddress != reinterpret_cast<std::uintptr_t>(program->codeBlocks) ||
        cached.codeSize != program->codeSize ||
        cached.resolvedProgramCounter >= program->codeSize)
        return false;

    g_signatureRevalidations.fetch_add(1, std::memory_order_relaxed);
    if (!PatternMatchesAt(program, cached.signatureProgramCounter, descriptor.bytePattern))
        return false;

    switch (descriptor.locator) {
    case GTA_Script_Function_Locator_Kind::FixedProgramCounter:
        return cached.signatureProgramCounter == descriptor.programCounter &&
               cached.resolvedProgramCounter == descriptor.programCounter;
    case GTA_Script_Function_Locator_Kind::UniqueBytePattern: {
        const auto entry = ApplyAdjustment(cached.signatureProgramCounter,
                                           descriptor.entryAdjustment,
                                           program->codeSize);
        return entry && *entry == cached.resolvedProgramCounter;
    }
    case GTA_Script_Function_Locator_Kind::UniqueBytePatternU24Target: {
        const auto entry = ResolveU24Target(program,
                                            cached.signatureProgramCounter,
                                            descriptor.targetOperandOffset,
                                            descriptor.entryAdjustment);
        return entry && *entry == cached.resolvedProgramCounter;
    }
    }
    return false;
}

bool ResolveProgramCounter(
    const ScriptFunctionRuntime& runtime,
    const GTA_Script_Function_Descriptor& descriptor,
    GTA_Script_Program_View* program,
    ResolvedFunction& resolved,
    GTA_Script_Function_Invoke_Snapshot& result) noexcept
{
    ScriptFunctionCacheEntry cached{};
    bool hadCachedEntry = false;
    {
        try {
            std::scoped_lock lock(g_cacheMutex);
            const auto it = g_pcCache.find(descriptor.id);
            if (it != g_pcCache.end()) {
                cached = it->second;
                hadCachedEntry = true;
            }
        } catch (...) {
        }
    }

    if (hadCachedEntry && ValidateCachedEntry(descriptor, program, cached)) {
        g_cacheHits.fetch_add(1, std::memory_order_relaxed);
        resolved.signatureProgramCounter = cached.signatureProgramCounter;
        resolved.resolvedProgramCounter = cached.resolvedProgramCounter;
        resolved.cacheHit = true;
        return true;
    }

    g_cacheMisses.fetch_add(1, std::memory_order_relaxed);
    if (hadCachedEntry)
        InvalidateCacheEntry(descriptor.id);

    std::uint32_t signaturePc = descriptor.programCounter;
    std::optional<std::uint32_t> entryPc;

    switch (descriptor.locator) {
    case GTA_Script_Function_Locator_Kind::FixedProgramCounter:
        if (signaturePc >= program->codeSize) {
            result.status = GTA_Script_Function_Invoke_Status::ProgramCounterOutOfRange;
            result.detail = "Fixed program counter lies outside the loaded script bytecode";
            return false;
        }
        if (!PatternMatchesAt(program, signaturePc, descriptor.bytePattern)) {
            result.status = GTA_Script_Function_Invoke_Status::SignatureMismatch;
            result.detail = "Loaded script bytes do not match the descriptor signature";
            return false;
        }
        entryPc = signaturePc;
        break;

    case GTA_Script_Function_Locator_Kind::UniqueBytePattern:
    case GTA_Script_Function_Locator_Kind::UniqueBytePatternU24Target: {
        const auto scan = FindUniquePattern(program, descriptor.bytePattern);
        if (scan.state == PatternScanState::NotFound) {
            result.status = GTA_Script_Function_Invoke_Status::PatternNotFound;
            result.detail = "Byte pattern was not found in the loaded script";
            return false;
        }
        if (scan.state == PatternScanState::Multiple) {
            result.status = GTA_Script_Function_Invoke_Status::PatternNotUnique;
            result.detail = "Byte pattern matched more than once in the loaded script";
            return false;
        }
        if (scan.state == PatternScanState::Unreadable) {
            result.status = GTA_Script_Function_Invoke_Status::Failed;
            result.detail = "Loaded script bytecode could not be read safely during pattern resolution";
            return false;
        }
        signaturePc = scan.programCounter;
        if (descriptor.locator == GTA_Script_Function_Locator_Kind::UniqueBytePattern) {
            entryPc = ApplyAdjustment(signaturePc, descriptor.entryAdjustment, program->codeSize);
        } else {
            entryPc = ResolveU24Target(program,
                                       signaturePc,
                                       descriptor.targetOperandOffset,
                                       descriptor.entryAdjustment);
        }
        if (!entryPc) {
            result.status = GTA_Script_Function_Invoke_Status::ProgramCounterOutOfRange;
            result.detail = "Resolved function entry lies outside the loaded script bytecode";
            return false;
        }
        break;
    }
    }

    if (!entryPc || *entryPc >= program->codeSize) {
        result.status = GTA_Script_Function_Invoke_Status::ProgramCounterOutOfRange;
        result.detail = "Resolved function entry lies outside the loaded script bytecode";
        return false;
    }

    resolved.signatureProgramCounter = signaturePc;
    resolved.resolvedProgramCounter = *entryPc;
    resolved.cacheHit = false;

    try {
        const ScriptFunctionCacheEntry newEntry{
            DescriptorKey(descriptor),
            runtime.buildFingerprint,
            Joaat(descriptor.scriptName),
            reinterpret_cast<std::uintptr_t>(program),
            reinterpret_cast<std::uintptr_t>(program->codeBlocks),
            program->codeSize,
            signaturePc,
            *entryPc,
        };
        std::scoped_lock lock(g_cacheMutex);
        g_pcCache.insert_or_assign(descriptor.id, newEntry);
    } catch (...) {
        // Cache failure must never block a verified one-shot invocation.
    }
    return true;
}

GTA_Tls_Context_View* CurrentTls() noexcept
{
    const auto tlsArray = static_cast<std::uintptr_t>(__readgsqword(0x58));
    if (!IsReadableAddress(tlsArray, sizeof(void*)))
        return nullptr;

    GTA_Tls_Context_View* tls = nullptr;
    std::memcpy(&tls, reinterpret_cast<const void*>(tlsArray), sizeof(tls));
    if (!tls || !IsReadableAddress(reinterpret_cast<std::uintptr_t>(tls),
                                   sizeof(GTA_Tls_Context_View)))
        return nullptr;
    return tls;
}

std::string HexPc(std::uint32_t pc)
{
    std::ostringstream stream;
    stream << "0x" << std::hex << std::uppercase << pc;
    return stream.str();
}

GTA_Script_Value NormalizeReturnValue(GTA_Script_Value_Type type,
                                      std::uint64_t raw) noexcept
{
    switch (type) {
    case GTA_Script_Value_Type::Int32:
    case GTA_Script_Value_Type::UInt32:
    case GTA_Script_Value_Type::Float:
    case GTA_Script_Value_Type::Hash32:
        raw &= 0xFFFFFFFFULL;
        break;
    case GTA_Script_Value_Type::Bool:
        raw = (raw & 0xFFFFFFFFULL) != 0 ? 1ULL : 0ULL;
        break;
    default:
        break;
    }
    return GTA_Script_Value{type, raw};
}

GTA_Script_Function_Invoke_Snapshot Execute(
    const ScriptFunctionRuntime& runtime,
    const ScriptFunctionRequest& request) noexcept
{
    GTA_Script_Function_Invoke_Snapshot result{};
    result.requestId = request.requestId;
    result.status = GTA_Script_Function_Invoke_Status::Failed;
    result.descriptorId = request.descriptor.id;
    result.label = request.descriptor.label;
