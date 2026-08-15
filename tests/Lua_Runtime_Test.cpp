#include "Backend/Threading/IExecutor.hpp"
#include "Scripting/Lua/Lua_Runtime.hpp"

#include <atomic>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

namespace
{
class Test_Lua_Executor final : public Devilz::Backend::IExecutor
{
public:
    ~Test_Lua_Executor() override
    {
        if (m_worker.joinable()) {
            m_worker.request_stop();
            m_worker.join();
        }
    }

    std::string_view Name() const noexcept override
    {
        return "LuaTest";
    }

    Devilz::Backend::TaskId Submit(Devilz::Backend::Task task) override
    {
        if (!task)
            throw std::invalid_argument("Cannot submit an empty test task");

        if (m_worker.joinable())
            throw std::runtime_error("Test Lua executor already owns a task");

        const auto id = ++m_nextTask;
        m_worker = std::jthread([task = std::move(task)](std::stop_token) mutable {
            task();
        });
        return id;
    }

    std::size_t Pending() const noexcept override
    {
        return 0;
    }

private:
    Devilz::Backend::TaskId m_nextTask{};
    std::jthread m_worker;
};
}

int main()
{
    auto& runtime = Devilz::Scripting::Lua::Lua_Runtime::Instance();
    std::atomic_size_t loggedMessages{};

    auto testLogger = [&loggedMessages](
        Devilz::Backend::LogLevel,
        std::string,
        std::string) {
        loggedMessages.fetch_add(1, std::memory_order_relaxed);
    };

    Test_Lua_Executor executor;
    if (!runtime.Start(executor, testLogger)) {
        std::cerr << "Lua runtime failed to start: " << runtime.Status() << '\n';
        return 1;
    }

    const auto snapshot = runtime.Snapshot();
    if (!snapshot.ready || !snapshot.dedicatedThread) {
        std::cerr << "Lua runtime did not report dedicated-thread ownership\n";
        return 1;
    }

    if (snapshot.libraries < 3) {
        std::cerr << "Lua runtime did not register core/logger/events binding libraries\n";
        return 1;
    }

    const auto result = runtime.RunSelfTest();
    if (!result.succeeded) {
        std::cerr << "Lua runtime self-test failed: " << result.message << '\n';
        return 1;
    }

    if (loggedMessages.load(std::memory_order_relaxed) == 0) {
        std::cerr << "Lua logger binding did not use the injected log callback\n";
        return 1;
    }

    runtime.Shutdown();
    if (runtime.Ready()) {
        std::cerr << "Lua runtime remained ready after shutdown\n";
        return 1;
    }

    Test_Lua_Executor secondExecutor;
    if (!runtime.Start(secondExecutor, testLogger)) {
        std::cerr << "Lua runtime failed to restart: " << runtime.Status() << '\n';
        return 1;
    }

    const auto secondResult = runtime.RunSelfTest();
    if (!secondResult.succeeded) {
        std::cerr << "Lua runtime self-test failed after restart: " << secondResult.message << '\n';
        return 1;
    }

    runtime.Shutdown();
    return 0;
}
