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

bool SourceIdentityValid(const GTA_Script_Function_Descriptor& descriptor) noexcept
{
    if (!descriptor.sourceReference.empty())
        return true;
    return !descriptor.source.repository.empty() &&
           !descriptor.source.revision.empty() &&
           !descriptor.source.path.empty();
}

void EnsureDescriptorId(GTA_Script_Function_Descriptor& descriptor)
{
    if (!descriptor.id.empty())
        return;

    descriptor.id = "legacy:" + descriptor.scriptName + ":" + descriptor.label;
    if (!descriptor.sourceReference.empty())
        descriptor.id += ":" + descriptor.sourceReference;
}

bool ValueTypeValid(GTA_Script_Value_Type type, bool allowVoid) noexcept
{
    if (type == GTA_Script_Value_Type::Void)
        return allowVoid;
    switch (type) {
    case GTA_Script_Value_Type::Int32:
    case GTA_Script_Value_Type::UInt32:
    case GTA_Script_Value_Type::Int64:
    case GTA_Script_Value_Type::UInt64:
    case GTA_Script_Value_Type::Float:
    case GTA_Script_Value_Type::Bool:
    case GTA_Script_Value_Type::Hash32:
        return true;
    default:
        return false;
    }
}

bool DescriptorValid(const GTA_Script_Function_Descriptor& descriptor) noexcept
{
    if (descriptor.id.empty() || descriptor.label.empty() || descriptor.scriptName.empty() ||
        !SourceIdentityValid(descriptor) || !descriptor.verifiedSynchronous ||
        descriptor.expectedBuildFingerprint == 0)
        return false;

    if (descriptor.bytePattern.size() < MinPatternBytes ||
        descriptor.bytePattern.size() > MaxPatternBytes ||
        descriptor.abi.arguments.size() > MaxFunctionArguments ||
        !ValueTypeValid(descriptor.abi.returnType, true))
        return false;

    for (const auto type : descriptor.abi.arguments) {
        if (!ValueTypeValid(type, false))
            return false;
    }

    std::size_t exactCount = 0;
    for (const auto value : descriptor.bytePattern) {
        if (value < -1 || value > 0xFF)
            return false;
        if (value >= 0)
            ++exactCount;
    }
    if (exactCount < MinExactPatternBytes)
        return false;

    if (descriptor.locator == GTA_Script_Function_Locator_Kind::UniqueBytePatternU24Target) {
        const auto operand = static_cast<std::size_t>(descriptor.targetOperandOffset);
        if (operand + 3 > descriptor.bytePattern.size())
            return false;
    }
    return true;
}

std::optional<std::size_t> ValidateArguments(
    const GTA_Script_Function_Descriptor& descriptor,
    const std::vector<GTA_Script_Value>& arguments) noexcept
{
    if (arguments.size() != descriptor.abi.arguments.size())
        return std::numeric_limits<std::size_t>::max();

    for (std::size_t index = 0; index < arguments.size(); ++index) {
        if (!ValueTypeValid(arguments[index].type, false) ||
            arguments[index].type != descriptor.abi.arguments[index])
            return index;
        if (arguments[index].type == GTA_Script_Value_Type::Bool && arguments[index].slot > 1)
            return index;
    }
    return std::nullopt;
}

std::uint64_t HashBytes(std::uint64_t hash, const void* data, std::size_t size) noexcept
{
    constexpr std::uint64_t Prime = 1099511628211ULL;
    const auto* bytes = static_cast<const std::uint8_t*>(data);
    for (std::size_t index = 0; index < size; ++index) {
        hash ^= bytes[index];
        hash *= Prime;
    }
    return hash;
}

std::uint64_t HashString(std::uint64_t hash, std::string_view value) noexcept
{
    return HashBytes(hash, value.data(), value.size());
}

std::uint64_t DescriptorKey(const GTA_Script_Function_Descriptor& descriptor) noexcept
{
    std::uint64_t hash = 1469598103934665603ULL;
    hash = HashString(hash, descriptor.id);
    hash = HashString(hash, descriptor.scriptName);
    hash = HashBytes(hash, &descriptor.expectedBuildFingerprint,
                     sizeof(descriptor.expectedBuildFingerprint));
    hash = HashBytes(hash, &descriptor.expectedScriptCodeSize,
                     sizeof(descriptor.expectedScriptCodeSize));
    hash = HashBytes(hash, &descriptor.locator, sizeof(descriptor.locator));
    hash = HashBytes(hash, &descriptor.programCounter, sizeof(descriptor.programCounter));
    hash = HashBytes(hash, &descriptor.entryAdjustment, sizeof(descriptor.entryAdjustment));
    hash = HashBytes(hash, &descriptor.targetOperandOffset, sizeof(descriptor.targetOperandOffset));
    for (const auto byte : descriptor.bytePattern)
        hash = HashBytes(hash, &byte, sizeof(byte));
    for (const auto type : descriptor.abi.arguments)
        hash = HashBytes(hash, &type, sizeof(type));
    hash = HashBytes(hash, &descriptor.abi.returnType, sizeof(descriptor.abi.returnType));
    return hash;
}

GTA_Script_Thread_View* FindScriptThread(const ScriptFunctionRuntime& runtime,
                                         std::uint32_t scriptHash) noexcept
{
    if (!IsReadableAddress(runtime.scriptThreadsStorageAddress,
                           sizeof(GTA_Script_Thread_Array_View)))
        return nullptr;

    GTA_Script_Thread_Array_View threads{};
    std::memcpy(&threads,
                reinterpret_cast<const void*>(runtime.scriptThreadsStorageAddress),
                sizeof(threads));

    const auto count = std::min<std::size_t>(threads.size, MaxScriptThreads);
    if (!threads.data || count == 0 ||
        !IsReadableAddress(reinterpret_cast<std::uintptr_t>(threads.data),
                           count * sizeof(void*)))
        return nullptr;

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

GTA_Script_Program_View* FindScriptProgram(const ScriptFunctionRuntime& runtime,
                                           std::uint32_t scriptHash) noexcept
{
    if (!IsReadableAddress(runtime.programTableAddress,
                           ScriptProgramCount * sizeof(void*)))
        return nullptr;

    auto** programs = reinterpret_cast<GTA_Script_Program_View**>(runtime.programTableAddress);
    for (std::size_t index = 0; index < ScriptProgramCount; ++index) {
        GTA_Script_Program_View* program = nullptr;
        std::memcpy(&program, programs + index, sizeof(program));
        if (!program ||
            !IsReadableAddress(reinterpret_cast<std::uintptr_t>(program),
                               sizeof(GTA_Script_Program_View)))
            continue;
        if (program->nameHash == scriptHash && program->codeBlocks && program->codeSize != 0)
            return program;
    }
    return nullptr;
}

bool ResolveCodeByte(GTA_Script_Program_View* program,
                     std::uint32_t programCounter,
                     std::uint8_t& out) noexcept
{
    if (!program || !program->codeBlocks || programCounter >= program->codeSize)
        return false;

    const std::size_t pageIndex = programCounter / ScriptCodePageSize;
    const std::size_t pageOffset = programCounter % ScriptCodePageSize;
    const std::size_t pageCount =
        (static_cast<std::size_t>(program->codeSize) + ScriptCodePageSize - 1) /
        ScriptCodePageSize;
    if (pageIndex >= pageCount ||
        !IsReadableAddress(reinterpret_cast<std::uintptr_t>(program->codeBlocks),
                           pageCount * sizeof(std::uint8_t*)))
        return false;

    std::uint8_t* page = nullptr;
