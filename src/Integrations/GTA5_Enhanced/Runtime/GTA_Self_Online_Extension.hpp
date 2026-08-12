#pragma once

#include <cstdint>

namespace Devilz::Backend
{
class LoggerService;
}

namespace Devilz::Integrations::GTA5_Enhanced
{
class GTA_Native_Manager;

void ConfigureSelfOnlineExtension(
    std::uintptr_t scriptGlobalsAddress,
    std::uintptr_t scriptThreadsStorageAddress,
    Backend::LoggerService* logger) noexcept;

void ResetSelfOnlineExtension() noexcept;
void TickSelfOnlineExtension(GTA_Native_Manager& natives) noexcept;
}
