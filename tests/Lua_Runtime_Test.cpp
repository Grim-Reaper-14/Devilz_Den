#include "Backend/Threading/IExecutor.hpp"
#include "Scripting/Lua/Lua_Manager.hpp"
#include "Scripting/Lua/Lua_Runtime.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
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

struct Hot_Reload_Probe
{
    bool succeeded{};
    std::string message;
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

            const bool registriesPopulated =
                manager.Settings().CountByOwner(scriptId) == 1 &&
                manager.Features().CountByOwner(scriptId) == 1;

            const bool fingerprintReady =
                probe.fingerprint != 0 &&
                probe.contentHash != 0 &&
                probe.runtimeFingerprint == manager.Fingerprints().Runtime().value;

            if (!manager.Scripts().UnloadScript(scriptId)) {
                probe.message = "Lua script manager could not unload the registry test script";
                promise->set_value(std::move(probe));
                return;
            }

            const bool registriesCleaned =
                manager.Settings().CountByOwner(scriptId) == 0 &&
                manager.Features().CountByOwner(scriptId) == 0;

            probe.succeeded = fingerprintReady && registriesPopulated && registriesCleaned;
            if (!probe.succeeded) {
                if (!fingerprintReady)
                    probe.message = "Lua script fingerprint fields were not populated consistently";
                else if (!registriesPopulated)
                    probe.message = "Lua script settings/features were not registered for the script owner";
                else
                    probe.message = "Lua script settings/features survived script unload";
            }

            promise->set_value(std::move(probe));
        })) {
        return {false, "Lua runtime rejected the fingerprint test job"};
    }

    return future.get();
}

Hot_Reload_Probe ProbeHotReload(
    Devilz::Scripting::Lua::Lua_Runtime& runtime,
    std::filesystem::path path)
{
    auto promise = std::make_shared<std::promise<Hot_Reload_Probe>>();
    auto future = promise->get_future();

    if (!runtime.Submit([promise, path = std::move(path)](Devilz::Scripting::Lua::Lua_Manager& manager) {
            Hot_Reload_Probe probe;
            auto finish = [&](std::string message) {
                manager.HotReload().SetEnabled(true);
                probe.message = std::move(message);
                promise->set_value(std::move(probe));
            };

            manager.HotReload().SetEnabled(false);

            const std::string firstContent =
                "assert(devilz.settings.register('hot_value', 1))\n"
                "assert(devilz.features.register('hot_feature', false))\n";
            const std::string brokenContent =
                "assert(devilz.settings.register('hot_value', 99))\n"
                "assert(devilz.features.register('hot_feature', true))\n"
                "error('intentional hot reload failure')\n";
            const std::string recoveredContent =
                "assert(devilz.settings.register('hot_value', 2))\n"
                "assert(devilz.features.register('hot_feature', true))\n";

            if (!WriteScript(path, firstContent)) {
                finish("Unable to create Lua hot reload test script");
                return;
            }

            auto* script = manager.Scripts().LoadScript(path);
            if (!script || script->State() != Devilz::Scripting::Lua::Lua_Script_State::Running) {
                finish(script ? script->LastError() : "Lua hot reload test script was rejected");
                return;
            }

            const auto scriptId = script->GetId();
            const auto firstFingerprint = script->Fingerprint();
            const auto* firstValue = manager.Settings().Get(scriptId, "hot_value");
            const auto* firstInteger = firstValue
                ? std::get_if<std::int64_t>(firstValue)
                : nullptr;
            if (!firstInteger || *firstInteger != 1 || manager.Features().Enabled(scriptId, "hot_feature")) {
                manager.Scripts().UnloadScript(scriptId);
                finish("Lua hot reload baseline resources were incorrect");
                return;
            }

            if (!WriteScript(path, brokenContent)) {
                manager.Scripts().UnloadScript(scriptId);
                finish("Unable to write broken Lua hot reload revision");
                return;
            }

            if (manager.HotReload().ScanNow(manager.Scripts(), manager.Fingerprints()) != 0) {
                manager.Scripts().UnloadScript(scriptId);
                finish("Broken Lua revision incorrectly reported a successful reload");
                return;
            }

            script = manager.Scripts().FindScript(scriptId);
            if (!script || script->State() != Devilz::Scripting::Lua::Lua_Script_State::Error ||
                script->EngineId() != 0 || script->Fingerprint() == firstFingerprint) {
                manager.Scripts().UnloadScript(scriptId);
                finish("Failed Lua hot reload did not preserve owner/error fingerprint state");
                return;
            }

            const auto brokenFingerprint = script->Fingerprint();
            if (manager.Settings().CountByOwner(scriptId) != 0 ||
                manager.Features().CountByOwner(scriptId) != 0) {
                manager.Scripts().UnloadScript(scriptId);
                finish("Failed Lua hot reload leaked owner resources");
                return;
            }

            if (!WriteScript(path, recoveredContent)) {
                manager.Scripts().UnloadScript(scriptId);
                finish("Unable to write recovered Lua hot reload revision");
                return;
            }

            if (manager.HotReload().ScanNow(manager.Scripts(), manager.Fingerprints()) != 1) {
                manager.Scripts().UnloadScript(scriptId);
                finish("Recovered Lua revision did not reload");
                return;
            }

            script = manager.Scripts().FindScript(scriptId);
            if (!script || script->GetId() != scriptId ||
                script->State() != Devilz::Scripting::Lua::Lua_Script_State::Running ||
                script->Fingerprint() == brokenFingerprint) {
                manager.Scripts().UnloadScript(scriptId);
                finish("Lua hot reload did not recover the original script owner");
                return;
            }

            const auto* recoveredValue = manager.Settings().Get(scriptId, "hot_value");
            const auto* recoveredInteger = recoveredValue
                ? std::get_if<std::int64_t>(recoveredValue)
                : nullptr;
            if (!recoveredInteger || *recoveredInteger != 2 ||
                !manager.Features().Enabled(scriptId, "hot_feature")) {
                manager.Scripts().UnloadScript(scriptId);
                finish("Recovered Lua hot reload resources were incorrect");
                return;
            }

            const auto hotReload = manager.HotReload().Snapshot();
            if (hotReload.reloads < 1 || hotReload.failures < 1 || hotReload.scans < 2) {
                manager.Scripts().UnloadScript(scriptId);
                finish("Lua hot reload statistics were not updated");
                return;
            }

            if (!manager.Scripts().UnloadScript(scriptId) ||
                manager.Settings().CountByOwner(scriptId) != 0 ||
                manager.Features().CountByOwner(scriptId) != 0) {
                finish("Lua hot reload owner resources survived final unload");
                return;
            }

            manager.HotReload().SetEnabled(true);
            probe.succeeded = true;
            promise->set_value(std::move(probe));
        })) {
        return {false, "Lua runtime rejected the hot reload test job"};
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
    if (!snapshot.ready || !snapshot.dedicatedThread || !snapshot.hotReloadEnabled) {
        std::cerr << "Lua runtime did not report dedicated-thread hot reload ownership\n";
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

    if (snapshot.libraries < 5) {
        std::cerr << "Lua runtime did not register core/logger/events/settings/features binding libraries\n";
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
    const auto hotReloadScript = tempDirectory / ("devilz_lua_hot_reload_" + unique + ".lua");

    const std::string contentA =
        "assert(type(devilz.script) == 'table')\n"
        "assert(type(devilz.script.fingerprint) == 'string')\n"
        "assert(#devilz.script.fingerprint == 18)\n"
        "assert(type(devilz.script.content_hash) == 'string')\n"
        "assert(#devilz.script.content_hash == 18)\n"
        "assert(devilz.script.runtime_fingerprint == devilz.fingerprint)\n"
        "assert(devilz.settings.register('probe_integer', 41))\n"
        "assert(devilz.settings.type('probe_integer') == 'integer')\n"
        "assert(devilz.settings.set('probe_integer', 42))\n"
        "assert(devilz.settings.get('probe_integer') == 42)\n"
        "assert(devilz.features.register('probe_feature', false))\n"
        "assert(devilz.features.set('probe_feature', true))\n"
        "assert(devilz.features.enabled('probe_feature'))\n";
    const std::string contentB = contentA + "-- fingerprint content changed\n";

    if (!WriteScript(scriptA, contentA) || !WriteScript(scriptB, contentA)) {
        std::cerr << "Unable to create Lua fingerprint test scripts\n";
        return 1;
    }

    const auto first = ProbeScript(runtime, scriptA);
    const auto renamed = ProbeScript(runtime, scriptB);
    if (!first.succeeded || !renamed.succeeded) {
        std::cerr << "Lua script fingerprint/registry test failed: "
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
        std::cerr << "Changed Lua script fingerprint/registry test failed: " << changed.message << '\n';
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

    const auto hotReload = ProbeHotReload(runtime, hotReloadScript);
    if (!hotReload.succeeded) {
        std::cerr << "Lua hot reload test failed: " << hotReload.message << '\n';
        return 1;
    }

    std::error_code cleanupError;
    std::filesystem::remove(scriptA, cleanupError);
    cleanupError.clear();
    std::filesystem::remove(scriptB, cleanupError);
    cleanupError.clear();
    std::filesystem::remove(hotReloadScript, cleanupError);

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

    const auto secondSnapshot = runtime.Snapshot();
    if (!secondSnapshot.hotReloadEnabled || secondSnapshot.hotReloadScans != 0 ||
        secondSnapshot.hotReloads != 0 || secondSnapshot.hotReloadFailures != 0) {
        std::cerr << "Lua hot reload state did not reset across runtime restart\n";
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
