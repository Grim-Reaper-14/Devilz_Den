#include "Runtime_Manager.hpp"

#include "Backend/Logging/Sinks/ConsoleSink.hpp"
#include "Backend/Logging/Sinks/DebuggerSink.hpp"
#include "Backend/Logging/Sinks/FileSink.hpp"
#include "Backend/Process/Process_Module_Manager.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Bunker_Extension.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Business_Extension.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Casino_Extension.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Network_Session_Extension.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Random_Events_Extension.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Vehicle_Personal_Save.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_World_Environment_Extension.hpp"
#include "Scripting/Lua/Lua_Runtime.hpp"

#include <exception>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

namespace Devilz
{
namespace
{
using Backend::Process_Architecture;
using Integrations::GTA5_Enhanced::GTA_Module_Manager_State;
using Integrations::GTA5_Enhanced::GTA_Runtime_Target_Id;
using Integrations::GTA5_Enhanced::GTA_Runtime_Target_State;
using Integrations::GTA5_Enhanced::GTA_Target_Candidate_Kind;

const char* ArchitectureName(Process_Architecture architecture) noexcept
{
    switch (architecture) {
    case Process_Architecture::X86: return "x86";
    case Process_Architecture::X64: return "x64";
    case Process_Architecture::Arm64: return "ARM64";
    default: return "Unknown";
    }
}

const char* StateName(GTA_Module_Manager_State state) noexcept
{
    switch (state) {
    case GTA_Module_Manager_State::NotRunning: return "NotRunning";
    case GTA_Module_Manager_State::Detected: return "Detected";
    case GTA_Module_Manager_State::BuildIdentified: return "BuildIdentified";
    case GTA_Module_Manager_State::RuntimeUnverified: return "RuntimeUnverified";
    case GTA_Module_Manager_State::Unsupported: return "Unsupported";
    case GTA_Module_Manager_State::Supported: return "Supported";
    case GTA_Module_Manager_State::RuntimeReady: return "RuntimeReady";
    case GTA_Module_Manager_State::Failed: return "Failed";
    default: return "Unknown";
    }
}

const char* TargetStateName(GTA_Runtime_Target_State state) noexcept
{
    switch (state) {
    case GTA_Runtime_Target_State::Unknown: return "Unknown";
    case GTA_Runtime_Target_State::Missing: return "Missing";
    case GTA_Runtime_Target_State::Located: return "Located";
    case GTA_Runtime_Target_State::StructurallyValidated: return "StructurallyValidated";
    case GTA_Runtime_Target_State::Validated: return "Validated";
    case GTA_Runtime_Target_State::Failed: return "Failed";
    default: return "Unknown";
    }
}

const char* CandidateKindName(GTA_Target_Candidate_Kind kind) noexcept
{
    switch (kind) {
    case GTA_Target_Candidate_Kind::DirectData: return "DirectData";
    case GTA_Target_Candidate_Kind::PointerStorage: return "PointerStorage";
    case GTA_Target_Candidate_Kind::CodeSite: return "CodeSite";
    default: return "Unknown";
    }
}

bool TargetResolved(GTA_Runtime_Target_State state) noexcept
{
    return state == GTA_Runtime_Target_State::Located ||
           state == GTA_Runtime_Target_State::StructurallyValidated ||
           state == GTA_Runtime_Target_State::Validated;
}

std::string Hex(std::uint64_t value)
{
    std::ostringstream stream;
    stream << "0x" << std::uppercase << std::hex << value;
    return stream.str();
}
}

Runtime_Manager::~Runtime_Manager()
{
    Stop();
}

bool Runtime_Manager::Start(const std::filesystem::path& logPath)
{
    bool expected = false;
    if (!m_running.compare_exchange_strong(expected, true))
        return true;

    try {
        m_logger.AddSink(std::make_unique<Backend::ConsoleSink>());
        m_logger.AddSink(std::make_unique<Backend::DebuggerSink>());
        m_logger.AddSink(std::make_unique<Backend::FileSink>(logPath));
        m_logger.Start();
        m_logger.Log(Backend::LogLevel::Info,
                     "Devilz_Den DLL runtime starting | Log: " + logPath.string(),
                     "Runtime");

        m_threads.Start();
        m_threads.Workers().Submit([this] {
            m_logger.Log(Backend::LogLevel::Debug, "Worker executor is operational", "Threading");
        });
        m_threads.IO().Submit([this] {
            m_logger.Log(Backend::LogLevel::Debug, "IO executor is operational", "Threading");
        });

        auto& luaRuntime = Scripting::Lua::Lua_Runtime::Instance();
        auto& luaExecutor = m_threads.CreateDedicated("Lua");
        if (luaRuntime.Start(
                luaExecutor,
                [this](Backend::LogLevel level, std::string message, std::string service) {
                    m_logger.Log(level, std::move(message), std::move(service));
                })) {
            m_logger.Log(
                Backend::LogLevel::Info,
                "Lua runtime started on dedicated backend executor",
                "Lua");
        } else {
            m_logger.Log(
                Backend::LogLevel::Warning,
                "Lua runtime unavailable: " + luaRuntime.Status(),
                "Lua");
        }

        const auto refreshed = m_gta.Refresh();
        if (!refreshed) {
            m_logger.Log(Backend::LogLevel::Error,
                         "GTA detection failed: " + refreshed.Failure().Message(),
                         "GTA5_Enhanced");
        } else {
            LogGTAStatus(refreshed.Value());
            InitializeNativeManager(refreshed.Value());
            InitializeGameThreadBridge(refreshed.Value());

            if (refreshed.Value().state == GTA_Module_Manager_State::RuntimeReady) {
                m_frontend.Start(refreshed.Value(), m_logger);
            } else {
                m_logger.Log(
                    Backend::LogLevel::Warning,
                    "Frontend bootstrap skipped: GTA runtime is not RuntimeReady",
                    "GTA5_Enhanced.Frontend");
            }
        }

        m_logger.Log(Backend::LogLevel::Info, "Devilz_Den DLL bootstrap completed", "Runtime");
        return true;
    } catch (const std::exception& exception) {
        m_logger.Log(Backend::LogLevel::Error,
                     std::string("DLL runtime startup threw an exception: ") + exception.what(),
                     "Runtime");
    } catch (...) {
        m_logger.Log(Backend::LogLevel::Error,
                     "DLL runtime startup threw an unknown exception",
                     "Runtime");
    }

    Stop();
    return false;
}

void Runtime_Manager::Stop() noexcept
{
    if (!m_running.exchange(false))
        return;

    try {
        m_frontend.Stop();
        m_gameThreadBridge.Uninstall();
        Integrations::GTA5_Enhanced::ResetVehiclePersonalSave();
        Integrations::GTA5_Enhanced::ResetBusinessExtension();
        Integrations::GTA5_Enhanced::ResetCasinoExtension();
        Integrations::GTA5_Enhanced::ResetWorldEnvironmentExtension();
        m_scriptGlobals.Clear();
        m_natives.Reset();
        Scripting::Lua::Lua_Runtime::Instance().Shutdown();
        m_threads.Stop();
        m_logger.Log(Backend::LogLevel::Info, "Devilz_Den DLL runtime stopped cleanly", "Runtime");
        m_logger.Flush();
        m_logger.Stop();
    } catch (...) {
        // Shutdown is best-effort and must never escape across the DLL boundary.
    }
}

void Runtime_Manager::InitializeNativeManager(
    const Integrations::GTA5_Enhanced::GTA_Module_Status& status)
{
    if (!status.process || !status.build || !status.targetReport) {
        m_logger.Log(Backend::LogLevel::Warning,
                     "Native manager unavailable: GTA process, build, or target report is missing",
                     "GTA5_Enhanced.Natives");
        return;
    }

    std::uintptr_t initNativeTables = 0;
    for (const auto& target : status.targetReport->targets) {
        if (target.status.id == GTA_Runtime_Target_Id::InitNativeTables &&
            target.status.state == GTA_Runtime_Target_State::Validated) {
            initNativeTables = target.address;
            break;
        }
    }

    if (initNativeTables == 0) {
        m_logger.Log(Backend::LogLevel::Warning,
                     "Native manager unavailable: InitNativeTables is not semantically validated",
                     "GTA5_Enhanced.Natives");
        return;
    }

    Backend::Process_Module_Manager modules(status.process->pid);
    const auto module = modules.Find("GTA5_Enhanced.exe");
    if (!module) {
        m_logger.Log(Backend::LogLevel::Warning,
                     "Native manager unavailable: GTA module image could not be resolved",
                     "GTA5_Enhanced.Natives");
        return;
    }

    const auto nativeStatus = m_natives.Initialize(
        initNativeTables,
        module.Value().baseAddress,
        module.Value().imageSize,
        status.build->fingerprint);

    m_logger.Log(
        nativeStatus.ready ? Backend::LogLevel::Info : Backend::LogLevel::Warning,
        std::string("State: ") + (nativeStatus.ready ? "Ready" : "Unavailable") +
            " | CachedHandlers: " + std::to_string(nativeStatus.cachedHandlers) +
            "/" + std::to_string(nativeStatus.requestedHandlers) +
            " | " + nativeStatus.detail,
        "GTA5_Enhanced.Natives");
}

void Runtime_Manager::InitializeGameThreadBridge(
    const Integrations::GTA5_Enhanced::GTA_Module_Status& status)
{
    using Integrations::GTA5_Enhanced::ConfigureBunkerGlobals;
    using Integrations::GTA5_Enhanced::ConfigureBusinessExtension;
    using Integrations::GTA5_Enhanced::ConfigureCasinoExtension;
    using Integrations::GTA5_Enhanced::ConfigureNetworkSessionExtension;
    using Integrations::GTA5_Enhanced::ConfigureRandomEventsExtension;
    using Integrations::GTA5_Enhanced::ConfigureVehiclePersonalSave;
    using Integrations::GTA5_Enhanced::ResetVehiclePersonalSave;
    using Integrations::GTA5_Enhanced::ConfigureWorldEnvironmentExtension;
    using Integrations::GTA5_Enhanced::ResetBusinessExtension;
    using Integrations::GTA5_Enhanced::ResetCasinoExtension;
    using Integrations::GTA5_Enhanced::ResetNetworkSessionExtension;
    using Integrations::GTA5_Enhanced::ResetRandomEventsExtension;
    using Integrations::GTA5_Enhanced::ResetWorldEnvironmentExtension;

    ResetBusinessExtension();
    ResetCasinoExtension();
    ResetNetworkSessionExtension();
    ResetRandomEventsExtension();
    ResetVehiclePersonalSave();
    ResetWorldEnvironmentExtension();
    m_scriptGlobals.Clear();

    if (!m_natives.Ready() || !status.targetReport) {
        m_logger.Log(Backend::LogLevel::Warning,
                     "Game-thread native bridge unavailable: native manager or target report is not ready",
                     "GTA5_Enhanced.Natives");
        return;
    }

    std::uintptr_t runScriptThreads = 0;
    std::uintptr_t scriptThreadsStorage = 0;
    std::uintptr_t expectedDispatch = 0;
    std::uintptr_t scriptGlobals = 0;
    std::uintptr_t validatedScriptGlobals = 0;
    std::uintptr_t programTable = 0;
    std::uintptr_t scriptVm = 0;
    std::uintptr_t isSessionStarted = 0;

    for (const auto& target : status.targetReport->targets) {
        if (target.status.id == GTA_Runtime_Target_Id::RunScriptThreads &&
            target.status.state == GTA_Runtime_Target_State::Validated) {
            runScriptThreads = target.address;
        } else if (target.status.id == GTA_Runtime_Target_Id::ScriptThreads &&
                   target.status.state == GTA_Runtime_Target_State::Validated) {
            scriptThreadsStorage = target.address;
            expectedDispatch = target.evidence.objectDominantFirstQwordAddress;
        } else if (TargetResolved(target.status.state)) {
            if (target.status.id == GTA_Runtime_Target_Id::ScriptGlobals) {
                scriptGlobals = target.address;
                if (target.status.state == GTA_Runtime_Target_State::Validated)
                    validatedScriptGlobals = target.address;
            } else if (target.status.id == GTA_Runtime_Target_Id::ProgramTable) {
                programTable = target.address;
            } else if (target.status.id == GTA_Runtime_Target_Id::ScriptVM) {
                scriptVm = target.address;
            } else if (target.status.id == GTA_Runtime_Target_Id::IsSessionStarted) {
                isSessionStarted = target.address;
            }
        }
    }

    bool scriptGlobalsReady = false;
    if (validatedScriptGlobals != 0 && status.build) {
        auto configured = m_scriptGlobals.ConfigureFromTable(
            Backend::Pointer(validatedScriptGlobals),
            status.build->fingerprint);
        if (configured) {
            scriptGlobalsReady = true;
            m_logger.Log(
                Backend::LogLevel::Info,
                "Validated script global table configured for queued business, Casino, and World actions",
                "GTA5_Enhanced.ScriptGlobals");
        } else {
            m_logger.Log(
                Backend::LogLevel::Warning,
                "Script globals unavailable: " + configured.Failure().Message(),
                "GTA5_Enhanced.ScriptGlobals");
        }
    } else {
        m_logger.Log(
            Backend::LogLevel::Warning,
            "Script globals unavailable: ScriptGlobals is not semantically validated",
            "GTA5_Enhanced.ScriptGlobals");
    }

    if (runScriptThreads == 0 || scriptThreadsStorage == 0 || expectedDispatch == 0) {
        m_logger.Log(Backend::LogLevel::Warning,
                     "Game-thread native bridge unavailable: RunScriptThreads or ScriptThreads is not semantically validated",
                     "GTA5_Enhanced.Natives");
        return;
    }

    ConfigureCasinoExtension(
        scriptGlobalsReady ? &m_scriptGlobals : nullptr,
        scriptThreadsStorage,
        status.build ? status.build->fingerprint : 0,
        &m_logger);

    if (!m_gameThreadBridge.Install(
            runScriptThreads,
            scriptThreadsStorage,
            expectedDispatch,
            m_natives,
            m_logger)) {
        ResetCasinoExtension();
        m_logger.Log(Backend::LogLevel::Warning,
                     "RunScriptThreads bridge installation failed closed",
                     "GTA5_Enhanced.Natives");
        return;
    }

    ConfigureVehiclePersonalSave(
        scriptGlobals,
        programTable,
        scriptThreadsStorage,
        scriptVm,
        isSessionStarted,
        &m_logger);

    ConfigureBunkerGlobals(
        scriptGlobalsReady ? &m_scriptGlobals : nullptr,
        status.build ? status.build->fingerprint : 0);

    if (scriptGlobalsReady && status.build) {
        ConfigureBusinessExtension(&m_scriptGlobals, status.build->fingerprint);
        ConfigureWorldEnvironmentExtension(&m_scriptGlobals, status.build->fingerprint);
    }

    ConfigureNetworkSessionExtension(
        scriptGlobals,
        programTable,
        scriptThreadsStorage,
        scriptVm,
        &m_logger);
    ConfigureRandomEventsExtension(
        scriptGlobals,
        programTable,
        scriptThreadsStorage,
        scriptVm,
        status.build ? status.build->fingerprint : 0,
        &m_logger);
}

void Runtime_Manager::LogGTAStatus(const Integrations::GTA5_Enhanced::GTA_Module_Status& status)
{
    m_logger.Log(Backend::LogLevel::Info,
                 std::string("State: ") + StateName(status.state) + " - " + status.detail,
                 "GTA5_Enhanced");

    std::uintptr_t moduleBase = 0;

    if (status.process) {
        m_logger.Log(Backend::LogLevel::Info,
                     "PID: " + std::to_string(status.process->pid) +
                         " | Architecture: " + ArchitectureName(status.process->architecture),
                     "GTA5_Enhanced");
        if (!status.process->executablePath.empty())
            m_logger.Log(Backend::LogLevel::Info,
                         "Executable: " + status.process->executablePath.string(),
                         "GTA5_Enhanced");

        Backend::Process_Module_Manager modules(status.process->pid);
        auto module = modules.Find("GTA5_Enhanced.exe");
        if (module) {
            moduleBase = module.Value().baseAddress;
            m_logger.Log(Backend::LogLevel::Info,
                         "ModuleBase: " + Hex(moduleBase) +
                             " | ImageSize: " + Hex(module.Value().imageSize),
                         "GTA5_Enhanced");
        }
    }

    if (status.build) {
        m_logger.Log(Backend::LogLevel::Info,
                     "Build: " + status.build->version +
                         " | Fingerprint: " + Hex(status.build->fingerprint),
                     "GTA5_Enhanced");
    }

    if (!status.targetReport)
        return;

    m_logger.Log(Backend::LogLevel::Info,
                 "Runtime targets: " + std::to_string(status.targetReport->LocatedCount()) +
                     " located | " + std::to_string(status.targetReport->StructurallyValidatedCount()) +
                     " structurally validated | " +
                     std::to_string(status.targetReport->ValidatedCount()) + " semantically validated",
                 "GTA5_Enhanced");

    for (const auto& target : status.targetReport->targets) {
        std::string message = target.status.name + ": " + TargetStateName(target.status.state) +
                              " | Kind: " + CandidateKindName(target.candidateKind);
        if (target.address != 0) {
            message += " | Address: " + Hex(target.address);
            if (moduleBase != 0 && target.address >= moduleBase)
                message += " | RVA: " + Hex(target.address - moduleBase);
        }
        if (!target.status.detail.empty())
            message += " | " + target.status.detail;
        m_logger.Log(Backend::LogLevel::Info, std::move(message), "GTA5_Enhanced.Targets");

        if (target.address != 0 && !target.evidence.summary.empty()) {
            m_logger.Log(Backend::LogLevel::Info,
                         target.status.name + ": " + target.evidence.summary,
                         "GTA5_Enhanced.Evidence");
        }
    }
}
}
