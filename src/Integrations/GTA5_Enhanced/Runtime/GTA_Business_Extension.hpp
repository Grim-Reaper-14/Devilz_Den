#pragma once

#include <cstdint>

namespace Devilz::Integrations::GTA5_Enhanced
{
class GTA_Native_Manager;
class Script_Global_Manager;

void ConfigureBusinessExtension(
    Script_Global_Manager* globals,
    std::uint64_t buildFingerprint) noexcept;

void ResetBusinessExtension() noexcept;

void TickBusinessExtension(GTA_Native_Manager& natives) noexcept;
}
