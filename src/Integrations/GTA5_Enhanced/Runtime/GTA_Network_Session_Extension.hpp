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
    std::uintptr_t programTableAddress,
    std::uintptr_t scriptThreadsStorageAddress,
    std::uintptr_t scriptVmAddress,
    Backend::LoggerService* logger) noexcept;

void ResetNetworkSessionExtension() noexcept;
void TickNetworkSessionExtension() noexcept;
}
