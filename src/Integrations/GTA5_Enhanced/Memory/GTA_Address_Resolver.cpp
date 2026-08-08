#include "GTA_Address_Resolver.hpp"

#include <cstring>

namespace Devilz::Integrations::GTA5_Enhanced
{
using namespace Devilz::Backend;

Result<std::uintptr_t> GTA_Address_Resolver::Resolve(
    const Process_Memory_Reader& reader,
    std::uintptr_t matchAddress,
    const GTA_Address_Resolve_Rule& rule)
{
    if (matchAddress == 0)
        return Result<std::uintptr_t>::Failure(
            Error(ErrorCode::InvalidArgument, ErrorCategory::Runtime, "Cannot resolve a null pattern match"));

    switch (rule.mode) {
    case GTA_Address_Resolve_Mode::Direct:
        return Result<std::uintptr_t>::Success(matchAddress);

    case GTA_Address_Resolve_Mode::AddOffset:
        return Result<std::uintptr_t>::Success(
            static_cast<std::uintptr_t>(static_cast<std::intptr_t>(matchAddress) + rule.offset));

    case GTA_Address_Resolve_Mode::RipRelative32: {
        if (rule.instructionSize == 0)
            return Result<std::uintptr_t>::Failure(
                Error(ErrorCode::InvalidArgument, ErrorCategory::Runtime,
                    "RIP-relative rule requires a non-zero instruction size"));

        const auto displacementAddress = static_cast<std::uintptr_t>(
            static_cast<std::intptr_t>(matchAddress) + rule.displacementOffset);
        auto bytes = reader.Read(displacementAddress, sizeof(std::int32_t));
        if (!bytes)
            return Result<std::uintptr_t>::Failure(bytes.Failure());

        std::int32_t displacement = 0;
        std::memcpy(&displacement, bytes.Value().data(), sizeof(displacement));

        const auto instructionEnd = static_cast<std::intptr_t>(matchAddress) +
                                    static_cast<std::intptr_t>(rule.instructionSize);
        const auto resolved = instructionEnd + static_cast<std::intptr_t>(displacement) + rule.offset;
        if (resolved <= 0)
            return Result<std::uintptr_t>::Failure(
                Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
                    "RIP-relative address resolved outside the valid user address range"));

        return Result<std::uintptr_t>::Success(static_cast<std::uintptr_t>(resolved));
    }
    }

    return Result<std::uintptr_t>::Failure(
        Error(ErrorCode::InvalidArgument, ErrorCategory::Runtime, "Unknown GTA address resolution mode"));
}
}
