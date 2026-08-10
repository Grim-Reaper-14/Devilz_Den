#include "DLL_Bootstrap.hpp"

#include "Runtime_Manager.hpp"

#include <atomic>
#include <filesystem>
#include <string>

namespace Devilz::DLL_Bootstrap
{
namespace
{
std::atomic_bool g_stopRequested{false};
std::atomic_bool g_running{false};
HMODULE g_module = nullptr;

std::filesystem::path ResolveLogPath(HMODULE module)
{
    std::wstring buffer(32768, L'\0');
    const DWORD length = ::GetModuleFileNameW(module, buffer.data(), static_cast<DWORD>(buffer.size()));

    if (length == 0 || length >= buffer.size())
        return std::filesystem::path("logs") / "Devilz_Den.log";

    buffer.resize(length);
    return std::filesystem::path(buffer).parent_path() / "logs" / "Devilz_Den.log";
}

DWORD WINAPI BootstrapThread(void* parameter)
{
    auto* module = static_cast<HMODULE>(parameter);
    ::OutputDebugStringA("[Devilz_Den] Bootstrap thread started\n");

    Runtime_Manager runtime;
    const auto logPath = ResolveLogPath(module);

    if (!runtime.Start(logPath)) {
        ::OutputDebugStringA("[Devilz_Den] Runtime startup failed\n");
        g_running.store(false);
        ::FreeLibraryAndExitThread(module, 1);
    }

    g_running.store(true);
    ::OutputDebugStringA("[Devilz_Den] Runtime started\n");

    while (!g_stopRequested.load())
        ::Sleep(50);

    runtime.Stop();
    g_running.store(false);
    ::OutputDebugStringA("[Devilz_Den] Runtime stopped\n");
    ::FreeLibraryAndExitThread(module, 0);
}
}

bool Attach(HMODULE module) noexcept
{
    if (!module || g_module)
        return false;

    g_module = module;
    g_stopRequested.store(false);

    HANDLE thread = ::CreateThread(nullptr, 0, &BootstrapThread, module, 0, nullptr);
    if (!thread) {
        g_module = nullptr;
        return false;
    }

    ::CloseHandle(thread);
    return true;
}

void RequestUnload() noexcept
{
    g_stopRequested.store(true);
}

bool Running() noexcept
{
    return g_running.load();
}
}
