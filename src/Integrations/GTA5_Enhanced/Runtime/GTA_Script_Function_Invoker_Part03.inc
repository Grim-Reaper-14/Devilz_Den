    std::memcpy(&page, program->codeBlocks + pageIndex, sizeof(page));
    if (!page ||
        !IsReadableAddress(reinterpret_cast<std::uintptr_t>(page + pageOffset), 1))
        return false;

    out = page[pageOffset];
    return true;
}

bool PatternMatchesAt(GTA_Script_Program_View* program,
                      std::uint32_t programCounter,
                      const std::vector<std::int16_t>& pattern) noexcept
{
    if (!program || !program->codeBlocks || pattern.empty() ||
        programCounter >= program->codeSize ||
        pattern.size() > static_cast<std::size_t>(program->codeSize - programCounter))
        return false;

    const std::size_t pageIndex = programCounter / ScriptCodePageSize;
    const std::size_t pageOffset = programCounter % ScriptCodePageSize;
    const std::size_t pageCount =
        (static_cast<std::size_t>(program->codeSize) + ScriptCodePageSize - 1) /
        ScriptCodePageSize;

    // Cached signatures overwhelmingly live inside a single code page. Validate
    // the page once and compare directly so a cache hit does not devolve into
    // one VirtualQuery per signature byte.
    const std::size_t pageStart = pageIndex * ScriptCodePageSize;
    const std::size_t pageSize = std::min<std::size_t>(
        program->codeSize - pageStart, ScriptCodePageSize);
    if (pageOffset + pattern.size() <= pageSize &&
        IsReadableAddress(reinterpret_cast<std::uintptr_t>(program->codeBlocks),
                          pageCount * sizeof(std::uint8_t*))) {
        std::uint8_t* page = nullptr;
        std::memcpy(&page, program->codeBlocks + pageIndex, sizeof(page));
        if (page && IsReadableAddress(reinterpret_cast<std::uintptr_t>(page), pageSize)) {
            for (std::size_t index = 0; index < pattern.size(); ++index) {
                if (pattern[index] >= 0 &&
                    page[pageOffset + index] != static_cast<std::uint8_t>(pattern[index]))
                    return false;
            }
            return true;
        }
    }

    // Boundary signatures are rare and small; use the page-aware byte resolver
    // only for that narrow case.
    for (std::size_t index = 0; index < pattern.size(); ++index) {
        if (pattern[index] < 0)
            continue;
        std::uint8_t value = 0;
        if (!ResolveCodeByte(program,
                             programCounter + static_cast<std::uint32_t>(index),
                             value) ||
            value != static_cast<std::uint8_t>(pattern[index]))
            return false;
    }
    return true;
}

PatternScanResult FindUniquePattern(
    GTA_Script_Program_View* program,
    const std::vector<std::int16_t>& pattern) noexcept
{
    g_patternScans.fetch_add(1, std::memory_order_relaxed);

    if (!program || !program->codeBlocks || pattern.empty() || pattern.size() > program->codeSize)
        return {};

    const std::size_t pageCount =
        (static_cast<std::size_t>(program->codeSize) + ScriptCodePageSize - 1) /
        ScriptCodePageSize;
    if (!IsReadableAddress(reinterpret_cast<std::uintptr_t>(program->codeBlocks),
                           pageCount * sizeof(std::uint8_t*)))
        return {PatternScanState::Unreadable, 0};

    std::size_t anchorIndex = 0;
    while (anchorIndex < pattern.size() && pattern[anchorIndex] < 0)
        ++anchorIndex;
    if (anchorIndex == pattern.size())
        return {PatternScanState::Multiple, 0};
    const auto anchor = static_cast<std::uint8_t>(pattern[anchorIndex]);

    bool found = false;
    std::uint32_t foundPc = 0;
    auto rememberMatch = [&](std::uint32_t pc) noexcept -> bool {
        if (found)
            return false;
        found = true;
        foundPc = pc;
        return true;
    };

    for (std::size_t pageIndex = 0; pageIndex < pageCount; ++pageIndex) {
        std::uint8_t* page = nullptr;
        std::memcpy(&page, program->codeBlocks + pageIndex, sizeof(page));
        if (!page)
            return {PatternScanState::Unreadable, 0};

        const std::size_t pageStart = pageIndex * ScriptCodePageSize;
        const std::size_t remaining = program->codeSize - pageStart;
        const std::size_t pageSize = std::min<std::size_t>(remaining, ScriptCodePageSize);
        if (!IsReadableAddress(reinterpret_cast<std::uintptr_t>(page), pageSize))
            return {PatternScanState::Unreadable, 0};

        if (pageSize >= pattern.size()) {
            const std::size_t lastLocal = pageSize - pattern.size();
            for (std::size_t local = 0; local <= lastLocal; ++local) {
                if (page[local + anchorIndex] != anchor)
                    continue;

                bool equal = true;
                for (std::size_t byte = 0; byte < pattern.size(); ++byte) {
                    if (pattern[byte] >= 0 &&
                        page[local + byte] != static_cast<std::uint8_t>(pattern[byte])) {
                        equal = false;
                        break;
                    }
                }
                if (equal && !rememberMatch(static_cast<std::uint32_t>(pageStart + local)))
                    return {PatternScanState::Multiple, 0};
            }
        }

        if (pageIndex + 1 < pageCount && pattern.size() > 1) {
            const std::size_t firstBoundary =
                pageSize > pattern.size() - 1 ? pageSize - (pattern.size() - 1) : 0;
            for (std::size_t local = firstBoundary; local < pageSize; ++local) {
                const auto pc = static_cast<std::uint32_t>(pageStart + local);
                if (pattern.size() > static_cast<std::size_t>(program->codeSize - pc))
                    continue;
                std::uint8_t anchorValue = 0;
                if (!ResolveCodeByte(program,
                                     pc + static_cast<std::uint32_t>(anchorIndex),
                                     anchorValue))
                    return {PatternScanState::Unreadable, 0};
                if (anchorValue != anchor)
                    continue;
                if (PatternMatchesAt(program, pc, pattern) && !rememberMatch(pc))
                    return {PatternScanState::Multiple, 0};
            }
        }
    }

    return found ? PatternScanResult{PatternScanState::Unique, foundPc}
                 : PatternScanResult{PatternScanState::NotFound, 0};
}

std::optional<std::uint32_t> ApplyAdjustment(
    std::uint32_t base,
    std::ptrdiff_t adjustment,
    std::uint32_t codeSize) noexcept
{
    const auto adjusted = static_cast<std::int64_t>(base) +
                          static_cast<std::int64_t>(adjustment);
    if (adjusted < 0 || adjusted >= static_cast<std::int64_t>(codeSize))
        return std::nullopt;
    return static_cast<std::uint32_t>(adjusted);
}

std::optional<std::uint32_t> ResolveU24Target(
    GTA_Script_Program_View* program,
    std::uint32_t signaturePc,
    std::uint8_t operandOffset,
    std::ptrdiff_t entryAdjustment) noexcept
{
    std::uint8_t b0 = 0;
    std::uint8_t b1 = 0;
    std::uint8_t b2 = 0;
    const auto operandPc = signaturePc + static_cast<std::uint32_t>(operandOffset);
    if (!ResolveCodeByte(program, operandPc, b0) ||
        !ResolveCodeByte(program, operandPc + 1U, b1) ||
        !ResolveCodeByte(program, operandPc + 2U, b2))
        return std::nullopt;

    const std::uint32_t target = static_cast<std::uint32_t>(b0) |
                                 (static_cast<std::uint32_t>(b1) << 8U) |
                                 (static_cast<std::uint32_t>(b2) << 16U);
    return ApplyAdjustment(target, entryAdjustment, program->codeSize);
}

void InvalidateCacheEntry(std::string_view descriptorId) noexcept
{
    try {
        std::scoped_lock lock(g_cacheMutex);
        g_pcCache.erase(std::string(descriptorId));
    } catch (...) {
    }
}

bool ValidateCachedEntry(
    const GTA_Script_Function_Descriptor& descriptor,
    GTA_Script_Program_View* program,
    const ScriptFunctionCacheEntry& cached) noexcept
{
