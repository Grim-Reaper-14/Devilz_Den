#include "Pattern_Batch_Scanner.hpp"

#include "Backend/Logging/Logger_Manager.hpp"

#include <chrono>
#include <sstream>

namespace Devilz::Backend
{
std::uint64_t Pattern_Batch_Scanner::FingerprintValue(const Module_Fingerprint& fingerprint) noexcept
{
    return fingerprint.imageHash ^ (static_cast<std::uint64_t>(fingerprint.timeDateStamp) << 32) ^ fingerprint.sizeOfImage;
}

Result<Pointer> Pattern_Batch_Scanner::ApplyTransform(Pointer match, const Pattern_Request& request) const
{
    if (request.transform == Pattern_Address_Transform::Match)
        return Result<Pointer>::Success(match);

    if (request.transform == Pattern_Address_Transform::AddOffset)
        return Result<Pointer>::Success(match.Add(request.offset));

    const auto instruction = match.Add(request.offset);
    const Memory_Range local{instruction.Address(), request.relativeInstructionSize};
    auto resolved = instruction.ResolveRelative32(request.relativeDisplacementOffset, request.relativeInstructionSize, local);
    if (!resolved)
        return Result<Pointer>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
            "Unable to resolve pattern relative address").With("Pattern", request.name));
    return Result<Pointer>::Success(*resolved);
}

Result<Pattern_Resolution> Pattern_Batch_Scanner::ResolveOne(const Module_Info& module, const Pattern_Request& request)
{
    const auto fingerprint = FingerprintValue(module.fingerprint);
    const Pattern_Cache_Key cacheKey{module.name, request.name, fingerprint};

    if (m_cache) {
        m_cache->InvalidateFingerprint(module.name, fingerprint);
        if (auto cached = m_cache->Find(cacheKey)) {
            if ((!request.validator || request.validator(*cached)) && cached->InRange(module.ImageRange()))
                return Result<Pattern_Resolution>::Success({request.name, *cached, true, {}});
            m_cache->Invalidate(module.name, request.name);
        }
    }

    auto compiled = Pattern::Compile(request.signature);
    if (!compiled)
        return Result<Pattern_Resolution>::Failure(compiled.Failure().With("PatternName", request.name));

    std::vector<Memory_Range> ranges;
    if (request.section) {
        Module_Manager modules;
        auto section = modules.FindSection(module, *request.section);
        if (!section) return Result<Pattern_Resolution>::Failure(section.Failure().With("PatternName", request.name));
        ranges.push_back(section.Value());
    } else {
        Module_Manager modules;
        ranges = modules.SelectSections(module, request.access);
    }

    Pattern_Scan_Stats aggregate{};
    const auto begin = std::chrono::steady_clock::now();
    for (const auto& range : ranges) {
        auto found = m_scanner.FindFirst(range, compiled.Value());
        const auto stats = m_scanner.LastStatistics();
        aggregate.bytesScanned += stats.bytesScanned;
        aggregate.candidateCount += stats.candidateCount;
        aggregate.matchCount += stats.matchCount;
        if (!found) continue;

        auto transformed = ApplyTransform(found.Value().address, request);
        if (!transformed) return Result<Pattern_Resolution>::Failure(transformed.Failure());
        if (request.validator && !request.validator(transformed.Value()))
            continue;

        aggregate.elapsed = std::chrono::steady_clock::now() - begin;
        if (m_cache) m_cache->Store(cacheKey, transformed.Value(), module.base);

        std::ostringstream message;
        message << "Resolved pattern '" << request.name << "' in " << module.name
                << " after scanning " << aggregate.bytesScanned << " bytes";
        Logger_Manager::Instance().Debug(message.str(), "Pattern_Batch_Scanner");
        return Result<Pattern_Resolution>::Success({request.name, transformed.Value(), false, aggregate});
    }

    aggregate.elapsed = std::chrono::steady_clock::now() - begin;
    return Result<Pattern_Resolution>::Failure(
        Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime, "Pattern was not found")
            .With("Pattern", request.name)
            .With("Module", module.name)
            .With("Signature", request.signature)
            .With("BytesScanned", std::to_string(aggregate.bytesScanned)));
}

Result<std::vector<Pattern_Resolution>> Pattern_Batch_Scanner::Resolve(
    const Module_Info& module, const std::vector<Pattern_Request>& requests)
{
    std::vector<Pattern_Resolution> resolutions;
    resolutions.reserve(requests.size());
    for (const auto& request : requests) {
        auto result = ResolveOne(module, request);
        if (!result) {
            Logger_Manager::Instance().LogError(LogLevel::Error, result.Failure(), "Pattern_Batch_Scanner");
            return Result<std::vector<Pattern_Resolution>>::Failure(result.Failure());
        }
        resolutions.push_back(std::move(result.Value()));
    }
    return Result<std::vector<Pattern_Resolution>>::Success(std::move(resolutions));
}
}
