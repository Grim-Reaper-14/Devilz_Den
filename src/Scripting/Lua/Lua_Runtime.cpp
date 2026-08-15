#include "Lua_Runtime.hpp"

#include "Backend/Threading/IExecutor.hpp"
#include "Fingerprint/Lua_Fingerprint.hpp"
#include "Lua_Manager.hpp"

#include <chrono>
#include <condition_variable>
#include <deque>
#include <exception>
#include <future>
#include <mutex>
#include <thread>
#include <utility>

namespace Devilz::Scripting::Lua
{
class Lua_Runtime::Impl final
{
public:
    void Publish(Lua_Manager& manager, bool threaded)
    {
        Lua_Runtime_Snapshot next;
        const auto hotReload = manager.HotReload().Snapshot();

        next.ready = manager.Ready();
        next.dedicatedThread = threaded;
        next.hotReloadEnabled = hotReload.enabled;
        next.runtimeFingerprint = manager.Fingerprints().Runtime().value;
        next.engines = manager.Engines().Count();
        next.scripts = manager.Scripts().Scripts().size();
        next.modules = manager.Modules().Count();
        next.libraries = manager.Libraries().Count();
        next.commands = manager.Commands().Count();
        next.events = manager.Events().Count();
        next.settings = manager.Settings().Count();
        next.features = manager.Features().Count();
        next.scheduledTasks = manager.ScheduledTaskCount();
        next.hotReloadScans = hotReload.scans;
        next.hotReloads = hotReload.reloads;
        next.hotReloadFailures = hotReload.failures;
        next.hotReloadStatus = hotReload.status;
        next.status = std::string{manager.Status()};

        std::scoped_lock lock(mutex);
        next.pendingJobs = jobs.size();
        snapshot = std::move(next);
    }

    void PublishStopped(std::string status)
    {
        Lua_Runtime_Snapshot next;
        next.status = std::move(status);

        std::scoped_lock lock(mutex);
        snapshot = std::move(next);
    }

    mutable std::mutex mutex;
    std::condition_variable cv;
    std::deque<Lua_Runtime_Task> jobs;
    Lua_Runtime_Snapshot snapshot;
    Lua_Log_Callback logger;
    bool serviceActive{};
    bool stopRequested{};
    bool initializationComplete{};
    bool synchronousMode{};
    std::thread::id serviceThreadId{};
};

Lua_Runtime& Lua_Runtime::Instance()
{
    static Lua_Runtime runtime;
    return runtime;
}

Lua_Runtime::Lua_Runtime()
    : m_impl(std::make_unique<Impl>())
{
}

Lua_Runtime::~Lua_Runtime()
{
    Shutdown();
}

void Lua_Runtime::Initialize()
{
    {
        std::scoped_lock lock(m_impl->mutex);
        if (m_impl->serviceActive || m_impl->synchronousMode)
            return;
    }

    auto& manager = Lua_Manager::Instance();
    manager.ConfigureServices({});
    manager.HotReload().Reset();
    manager.Initialize();
    m_impl->Publish(manager, false);

    std::scoped_lock lock(m_impl->mutex);
    m_impl->synchronousMode = true;
}

bool Lua_Runtime::Start(Backend::IExecutor& executor, Lua_Log_Callback logger)
{
    {
        std::unique_lock lock(m_impl->mutex);
        if (m_impl->serviceActive) {
            m_impl->cv.wait(lock, [this] { return m_impl->initializationComplete; });
            return m_impl->snapshot.ready;
        }
    }

    bool shutdownSynchronous = false;
    {
        std::scoped_lock lock(m_impl->mutex);
        shutdownSynchronous = m_impl->synchronousMode;
        m_impl->synchronousMode = false;
    }

    if (shutdownSynchronous)
        Lua_Manager::Instance().Shutdown();

    {
        std::scoped_lock lock(m_impl->mutex);
        m_impl->jobs.clear();
        m_impl->logger = std::move(logger);
        m_impl->stopRequested = false;
        m_impl->initializationComplete = false;
        m_impl->serviceActive = true;
        m_impl->serviceThreadId = {};
        m_impl->snapshot = {};
        m_impl->snapshot.dedicatedThread = true;
        m_impl->snapshot.status = "Starting dedicated Lua thread";
    }

    try {
        executor.Submit([this] { ServiceLoop(); });
    } catch (const std::exception& error) {
        std::scoped_lock lock(m_impl->mutex);
        m_impl->logger = {};
        m_impl->serviceActive = false;
        m_impl->initializationComplete = true;
        m_impl->snapshot.dedicatedThread = false;
        m_impl->snapshot.status = error.what();
        m_impl->cv.notify_all();
        return false;
    } catch (...) {
        std::scoped_lock lock(m_impl->mutex);
        m_impl->logger = {};
        m_impl->serviceActive = false;
        m_impl->initializationComplete = true;
        m_impl->snapshot.dedicatedThread = false;
        m_impl->snapshot.status = "Failed to submit dedicated Lua service";
        m_impl->cv.notify_all();
        return false;
    }

    std::unique_lock lock(m_impl->mutex);
    m_impl->cv.wait(lock, [this] { return m_impl->initializationComplete; });
    return m_impl->snapshot.ready;
}

bool Lua_Runtime::Submit(Lua_Runtime_Task task)
{
    if (!task)
        return false;

    {
        std::scoped_lock lock(m_impl->mutex);
        if (!m_impl->serviceActive || m_impl->stopRequested)
            return false;
        m_impl->jobs.push_back(std::move(task));
        m_impl->snapshot.pendingJobs = m_impl->jobs.size();
    }

    m_impl->cv.notify_one();
    return true;
}

void Lua_Runtime::Shutdown() noexcept
{
    try {
        bool synchronousMode = false;
        {
            std::unique_lock lock(m_impl->mutex);
            if (m_impl->serviceActive) {
                m_impl->stopRequested = true;
                m_impl->cv.notify_all();

                if (std::this_thread::get_id() != m_impl->serviceThreadId)
                    m_impl->cv.wait(lock, [this] { return !m_impl->serviceActive; });
                return;
            }

            synchronousMode = m_impl->synchronousMode;
            m_impl->synchronousMode = false;
        }

        if (synchronousMode) {
            auto& manager = Lua_Manager::Instance();
            manager.Shutdown();
            manager.ConfigureServices({});
            m_impl->PublishStopped("Stopped");
        }
    } catch (...) {
    }
}

bool Lua_Runtime::Ready() const
{
    std::scoped_lock lock(m_impl->mutex);
    return m_impl->snapshot.ready;
}

Lua_Runtime_Snapshot Lua_Runtime::Snapshot() const
{
    std::scoped_lock lock(m_impl->mutex);
    return m_impl->snapshot;
}

std::string Lua_Runtime::Status() const
{
    std::scoped_lock lock(m_impl->mutex);
    return m_impl->snapshot.status;
}

std::string_view Lua_Runtime::LuaVersion() const noexcept
{
    return Lua_Manager::Instance().LuaVersion();
}

std::string_view Lua_Runtime::Sol2Version() const noexcept
{
    return Lua_Manager::Instance().Sol2Version();
}

std::uint64_t Lua_Runtime::Fingerprint() const
{
    std::scoped_lock lock(m_impl->mutex);
    return m_impl->snapshot.runtimeFingerprint;
}

std::string Lua_Runtime::FingerprintHex() const
{
    return Lua_Fingerprint_Manager::ToHex(Fingerprint());
}

Lua_Self_Test_Result Lua_Runtime::RunSelfTest()
{
    bool threaded = false;
    bool onServiceThread = false;
    {
        std::scoped_lock lock(m_impl->mutex);
        threaded = m_impl->serviceActive;
        onServiceThread = threaded && std::this_thread::get_id() == m_impl->serviceThreadId;
    }

    if (!threaded || onServiceThread) {
        const auto result = Lua_Manager::Instance().RunSelfTest();
        return {result.succeeded, result.message};
    }

    auto promise = std::make_shared<std::promise<Lua_Self_Test_Result>>();
    auto future = promise->get_future();

    if (!Submit([promise](Lua_Manager& manager) {
            try {
                const auto result = manager.RunSelfTest();
                promise->set_value({result.succeeded, result.message});
            } catch (const std::exception& error) {
                promise->set_value({false, error.what()});
            } catch (...) {
                promise->set_value({false, "Unknown Lua self-test error"});
            }
        })) {
        return {false, "Lua service is not accepting jobs"};
    }

    return future.get();
}

void Lua_Runtime::ServiceLoop()
{
    Lua_Log_Callback logger;
    {
        std::scoped_lock lock(m_impl->mutex);
        m_impl->serviceThreadId = std::this_thread::get_id();
        logger = m_impl->logger;
    }

    auto& manager = Lua_Manager::Instance();
    bool initialized = false;
    std::string failureStatus;

    try {
        manager.ConfigureServices(std::move(logger));
        manager.HotReload().Reset();
        initialized = manager.Initialize();
        if (!initialized)
            failureStatus = std::string{manager.Status()};
        m_impl->Publish(manager, true);
    } catch (const std::exception& error) {
        failureStatus = error.what();
    } catch (...) {
        failureStatus = "Unknown Lua service initialization error";
    }

    {
        std::scoped_lock lock(m_impl->mutex);
        if (!failureStatus.empty()) {
            m_impl->snapshot.ready = false;
            m_impl->snapshot.status = failureStatus;
        }
        m_impl->initializationComplete = true;
        if (!initialized)
            m_impl->stopRequested = true;
    }
    m_impl->cv.notify_all();

    using namespace std::chrono_literals;

    while (initialized) {
        std::deque<Lua_Runtime_Task> jobs;
        bool shouldStop = false;

        {
            std::unique_lock lock(m_impl->mutex);
            m_impl->cv.wait_for(
                lock,
                10ms,
                [this] { return m_impl->stopRequested || !m_impl->jobs.empty(); });
            jobs.swap(m_impl->jobs);
            m_impl->snapshot.pendingJobs = 0;
            shouldStop = m_impl->stopRequested;
        }

        for (auto& job : jobs) {
            try {
                job(manager);
            } catch (...) {
            }
        }

        manager.HotReload().Tick(manager.Scripts(), manager.Fingerprints());
        manager.Tick();
        m_impl->Publish(manager, true);

        if (shouldStop)
            break;
    }

    manager.Shutdown();
    manager.ConfigureServices({});

    {
        std::scoped_lock lock(m_impl->mutex);
        m_impl->jobs.clear();
        m_impl->logger = {};
        m_impl->serviceThreadId = {};
        m_impl->serviceActive = false;
        m_impl->stopRequested = false;
        m_impl->snapshot = {};
        m_impl->snapshot.status = failureStatus.empty() ? "Stopped" : failureStatus;
    }
    m_impl->cv.notify_all();
}
}
