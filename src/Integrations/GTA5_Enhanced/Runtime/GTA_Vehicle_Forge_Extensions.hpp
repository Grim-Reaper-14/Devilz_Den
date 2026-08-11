#pragma once

namespace Devilz::Integrations::GTA5_Enhanced
{
class GTA_Native_Manager;

// The RunScriptThreads bridge supplies the validated live GTA thread for the
// duration of each extension tick. It is used only for scoped script-identity
// work such as explosive ammo and is cleared immediately after the tick.
void SetForgeExtensionScriptThread(void* scriptThread) noexcept;
void TickVehicleForgeExtensions(GTA_Native_Manager& natives) noexcept;
}
