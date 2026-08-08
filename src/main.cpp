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
    case GTA_Module_Manager_State::Unsupported: return "Unsupported";
    case GTA_Module_Manager_State::Supported: return "Supported";
    case GTA_Module_Manager_State::RuntimeReady: return "RuntimeReady";
    case GTA_Module_Manager_State::Failed: return "Failed";
    default: return "Unknown";
    }
}

std::string HexFingerprint(std::uint64_t fingerprint)
{
    std::ostringstream stream;
    stream << "0x" << std::uppercase << std::hex << fingerprint;
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
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(25));
    threads.Stop();
    logger.Log(LogLevel::Info, "Backend runtime foundation stopped cleanly", "Runtime");
    logger.Stop();
    return 0;
}
