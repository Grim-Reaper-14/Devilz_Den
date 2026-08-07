#include "Backend/Logging/LoggerService.hpp"
#include "Backend/Logging/Sinks/DebuggerSink.hpp"
#include "Backend/Logging/Sinks/FileSink.hpp"
#include "Backend/Threading/ThreadManager.hpp"

#include <chrono>
#include <memory>
#include <thread>

int main()
{
    using namespace Devilz::Backend;

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

    std::this_thread::sleep_for(std::chrono::milliseconds(25));
    threads.Stop();
    logger.Log(LogLevel::Info, "Backend runtime foundation stopped cleanly", "Runtime");
    logger.Stop();
    return 0;
}
