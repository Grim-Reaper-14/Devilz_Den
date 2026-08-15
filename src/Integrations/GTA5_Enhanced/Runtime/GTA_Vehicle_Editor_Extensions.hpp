#pragma once

namespace Devilz::Backend
{
class LoggerService;
}

namespace Devilz::Integrations::GTA5_Enhanced
{
class GTA_Native_Manager;

// The RunScriptThreads bridge supplies the validated live GTA thread for the
// duration of each extension tick. It is used only for scoped script-identity
// work such as explosive ammo and is cleared immediately after the tick.
void ConfigureVehicleEditorLogging(Backend::LoggerService* logger) noexcept;
void SetVehicleEditorScriptThread(void* scriptThread) noexcept;
void TickVehicleEditorExtensions(GTA_Native_Manager& natives) noexcept;
}
