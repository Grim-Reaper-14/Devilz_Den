#include "Backend/Threading/IExecutor.hpp"
#include "Scripting/Lua/Lua_Manager.hpp"
#include "Scripting/Lua/Lua_Runtime.hpp"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <memory>
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

struct Script_Fingerprint_Probe
{
    bool succeeded{};
    std::string message;
    std::uint64_t fingerprint{};
    std::uint64_t contentHash{};
    std::uint64_t runtimeFingerprint{};
};

bool WriteScript(const std::filesystem::path& path, std::string_view content)
{
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream)
        return false;
    stream.write(content.data(), static_cast<std::streamsize>(content.size()));
    return static_cast<bool>(stream);
}

Script_Fingerprint_Probe ProbeScript(
    Devilz::Scripting::Lua::Lua_Runtime& runtime,
    std::filesystem::path path)
{
    auto promise = std::make_shared<std::promise<Script_Fingerprint_Probe>>();
    auto future = promise->get_future();

    if (!runtime.Submit([promise, path = std::move(path)](Devilz::Scripting::Lua::Lua_Manager& manager) {
            Script_Fingerprint_Probe probe;
            auto* script = manager.Scripts().LoadScript(path);
            if (!script) {
                probe.message = "Lua script manager rejected the fingerprint test script";
                promise->set_value(std::move(probe));
                return;
            }

            const auto scriptId = script->GetId();
            if (script->State() != Devilz::Scripting::Lua::Lua_Script_State::Running) {
                probe.message = script->LastError();
                manager.Scripts().UnloadScript(scriptId);
                promise->set_value(std::move(probe));
                return;
            }

            probe.fingerprint = script->Fingerprint();
            probe.contentHash = script->ContentHash();
            probe.runtimeFingerprint = script->RuntimeFingerprint();
            probe.succeeded =
                probe.fingerprint != 0 &&
                probe.contentHash != 0 &&
                probe.runtimeFingerprint == manager.Fingerprints().Runtime().value;
            if (!probe.succeeded)
                probe.message = "Lua script fingerprint fields were not populated consistently";

            manager.Scripts().UnloadScript(scriptId);
            promise->set_value(std::move(probe));
        })) {
        return {false, "Lua runtime rejected the fingerprint test job"};
    }

    return future.get();
}
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

    if (snapshot.runtimeFingerprint == 0 || runtime.Fingerprint() != snapshot.runtimeFingerprint) {
        std::cerr << "Lua runtime fingerprint was not published through the runtime snapshot\n";
        return 1;
    }

    const auto runtimeFingerprint = snapshot.runtimeFingerprint;
    const auto runtimeFingerprintHex = runtime.FingerprintHex();
    if (runtimeFingerprintHex.size() != 18 || runtimeFingerprintHex.rfind("0x", 0) != 0) {
        std::cerr << "Lua runtime fingerprint hex formatting is invalid\n";
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

    const auto unique = std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    const auto tempDirectory = std::filesystem::temp_directory_path();
    const auto scriptA = tempDirectory / ("devilz_lua_fingerprint_a_" + unique + ".lua");
    const auto scriptB = tempDirectory / ("devilz_lua_fingerprint_b_" + unique + ".lua");

    const std::string contentA =
        "assert(type(devilz.script) == 'table')\n"
        "assert(type(devilz.script.fingerprint) == 'string')\n"
        "assert(#devilz.script.fingerprint == 18)\n"
        "assert(type(devilz.script.content_hash) == 'string')\n"
        "assert(#devilz.script.content_hash == 18)\n"
        "assert(devilz.script.runtime_fingerprint == devilz.fingerprint)\n";
    const std::string contentB = contentA + "-- fingerprint content changed\n";

    if (!WriteScript(scriptA, contentA) || !WriteScript(scriptB, contentA)) {
        std::cerr << "Unable to create Lua fingerprint test scripts\n";
        return 1;
    }

    const auto first = ProbeScript(runtime, scriptA);
    const auto renamed = ProbeScript(runtime, scriptB);
    if (!first.succeeded || !renamed.succeeded) {
        std::cerr << "Lua script fingerprint test failed: "
                  << (!first.succeeded ? first.message : renamed.message) << '\n';
        return 1;
    }

    if (first.contentHash != renamed.contentHash || first.fingerprint != renamed.fingerprint) {
        std::cerr << "Lua script fingerprint incorrectly depends on the script path\n";
        return 1;
    }

    if (!WriteScript(scriptA, contentB)) {
        std::cerr << "Unable to update Lua fingerprint test script\n";
        return 1;
    }

    const auto changed = ProbeScript(runtime, scriptA);
    if (!changed.succeeded) {
        std::cerr << "Changed Lua script fingerprint test failed: " << changed.message << '\n';
        return 1;
    }

    if (changed.contentHash == first.contentHash || changed.fingerprint == first.fingerprint) {
        std::cerr << "Lua script fingerprint did not change after the script contents changed\n";
        return 1;
    }

    if (changed.runtimeFingerprint != runtimeFingerprint) {
        std::cerr << "Lua script runtime fingerprint did not match the active Lua runtime\n";
        return 1;
    }

    std::error_code cleanupError;
    std::filesystem::remove(scriptA, cleanupError);
    cleanupError.clear();
    std::filesystem::remove(scriptB, cleanupError);

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

    if (runtime.Fingerprint() != runtimeFingerprint) {
        std::cerr << "Lua runtime fingerprint changed across an identical restart\n";
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
