#include "Backend/Logging/LoggerService.hpp"
#include "Backend/Logging/Sinks/DebuggerSink.hpp"
#include "Backend/Logging/Sinks/FileSink.hpp"
#include "Backend/Threading/ThreadManager.hpp"
#include "Integrations/GTA5_Enhanced/GTA_Module_Manager.hpp"

#include <chrono>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <thread>

namespace
{
using Devilz::Backend::Process_Architecture;
using Devilz::Integrations::GTA5_Enhanced::GTA_Module_Manager_State;
using Devilz::Integrations::GTA5_Enhanced::GTA_Runtime_Target_State;

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
    case GTA_Runtime_Target_State::Validated: return "Validated";
    case GTA_Runtime_Target_State::Failed: return "Failed";
    default: return "Unknown";
    }
}

std::string HexFingerprint(std::uint64_t fingerprint)
{
    std::ostringstream stream;
    stream << "0x" << std::uppercase << std::hex << fingerprint;
    return stream.str();
}

std::string HexAddress(std::uintptr_t address)
{
    std::ostringstream stream;
    stream << "0x" << std::uppercase << std::hex << address;
    return stream.str();
}
}

int main()
{
    using namespace Devilz::Backend;
    using namespace Devilz::Integrations::GTA5_Enhanced;

    LoggerService logger;
    logger.AddSink(std::make_unique<DebuggerSink>());
    logger.AddSink(std::make_unique<FileSink>("logs/Devilz_Den.log"));
    logger.Start();
    logger.Log(LogLevel::Info, "Backend runtime foundation starting", "Runtime");

    ThreadManager threads;
    threads.Start();
    threads.Workers().Submit([&logger] {
        logger.Log(LogLevel::Debug, "Worker executor is operational", "Threading");
    });
    threads.IO().Submit([&logger] {
        logger.Log(LogLevel::Debug, "IO executor is operational", "Threading");
    });

    GTA_Module_Manager gta;
    const auto refreshed = gta.Refresh();
    if (!refreshed) {
        logger.Log(LogLevel::Error,
                   "GTA detection failed: " + refreshed.Failure().Message(),
                   "GTA5_Enhanced");
    } else {
        const auto& status = refreshed.Value();
        logger.Log(LogLevel::Info,
                   std::string("State: ") + StateName(status.state) + " - " + status.detail,
                   "GTA5_Enhanced");

        if (status.process) {
            logger.Log(LogLevel::Info,
                       "PID: " + std::to_string(status.process->pid) +
                           " | Architecture: " + ArchitectureName(status.process->architecture),
                       "GTA5_Enhanced");
            if (!status.process->executablePath.empty())
                logger.Log(LogLevel::Info,
                           "Executable: " + status.process->executablePath.string(),
                           "GTA5_Enhanced");
        }

        if (status.build) {
            logger.Log(LogLevel::Info,
                       "Build: " + status.build->version +
                           " | Fingerprint: " + HexFingerprint(status.build->fingerprint),
                       "GTA5_Enhanced");
        }

        if (status.targetReport) {
            logger.Log(LogLevel::Info,
                       "Runtime targets: " + std::to_string(status.targetReport->LocatedCount()) +
                           " located | " + std::to_string(status.targetReport->ValidatedCount()) + " validated",
                       "GTA5_Enhanced");

            for (const auto& target : status.targetReport->targets) {
                std::string message = target.status.name + ": " + TargetStateName(target.status.state);
                if (target.address != 0)
                    message += " | Address: " + HexAddress(target.address);
                if (!target.status.detail.empty())
                    message += " | " + target.status.detail;
                logger.Log(LogLevel::Info, std::move(message), "GTA5_Enhanced.Targets");
            }
        }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(25));
    threads.Stop();
    logger.Log(LogLevel::Info, "Backend runtime foundation stopped cleanly", "Runtime");
    logger.Stop();
    return 0;
}
