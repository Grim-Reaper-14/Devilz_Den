#pragma once

namespace Devilz::Integrations::GTA5_Enhanced
{
class GTA_Native_Manager;

void ResetSelfOnlineExtension() noexcept;
void TickSelfOnlineExtension(GTA_Native_Manager& natives) noexcept;
}
