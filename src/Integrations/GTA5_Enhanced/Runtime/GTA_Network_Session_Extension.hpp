#pragma once

#include <cstdint>

namespace Devilz::Backend
{
class LoggerService;
}

namespace Devilz::Integrations::GTA5_Enhanced
{
void ConfigureNetworkSessionExtension(
    std::uintptr_t scriptGlobalsAddress,
    std::uint64_t buildFingerprint,
    Backend::LoggerService* logger) noexcept;

void ResetNetworkSessionExtension() noexcept;
void TickNetworkSessionExtension() noexcept;
}
