#include "GTA_Address_Resolver.hpp"

#include <cstring>

namespace Devilz::Integrations::GTA5_Enhanced
{
using namespace Devilz::Backend;

namespace
{
Result<std::uintptr_t> ResolveRipRelative32(
    const Process_Memory_Reader& reader,
    std::uintptr_t address,
    std::ptrdiff_t displacementOffset)
{
    const auto displacementAddressSigned = static_cast<std::intptr_t>(address) + displacementOffset;
    if (displacementAddressSigned <= 0)
        return Result<std::uintptr_t>::Failure(
            Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
                "RIP-relative displacement address is outside the valid user address range"));

    const auto displacementAddress = static_cast<std::uintptr_t>(displacementAddressSigned);
    auto bytes = reader.Read(displacementAddress, sizeof(std::int32_t));
    if (!bytes)
        return Result<std::uintptr_t>::Failure(bytes.Failure());

    std::int32_t displacement = 0;
    std::memcpy(&displacement, bytes.Value().data(), sizeof(displacement));

    const auto instructionEnd = displacementAddressSigned + static_cast<std::intptr_t>(sizeof(displacement));
    const auto resolved = instructionEnd + static_cast<std::intptr_t>(displacement);
    if (resolved <= 0)
        return Result<std::uintptr_t>::Failure(
            Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
                "RIP-relative address resolved outside the valid user address range"));

    return Result<std::uintptr_t>::Success(static_cast<std::uintptr_t>(resolved));
}
}

Result<std::uintptr_t> GTA_Address_Resolver::Resolve(
    const Process_Memory_Reader& reader,
    std::uintptr_t matchAddress,
    const GTA_Address_Resolve_Chain& chain)
{
    if (matchAddress == 0)
        return Result<std::uintptr_t>::Failure(
            Error(ErrorCode::InvalidArgument, ErrorCategory::Runtime, "Cannot resolve a null pattern match"));

    std::uintptr_t address = matchAddress;
    for (const auto& op : chain) {
        switch (op.type) {
        case GTA_Address_Resolve_Op_Type::Add: {
            const auto resolved = static_cast<std::intptr_t>(address) + op.offset;
            if (resolved <= 0)
                return Result<std::uintptr_t>::Failure(
                    Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
                        "Address add operation resolved outside the valid user address range"));
            address = static_cast<std::uintptr_t>(resolved);
            break;
        }

        case GTA_Address_Resolve_Op_Type::RipRelative32: {
            auto resolved = ResolveRipRelative32(reader, address, op.offset);
            if (!resolved)
                return resolved;
            address = resolved.Value();
            break;
        }
        }
    }

    return Result<std::uintptr_t>::Success(address);
}
}
