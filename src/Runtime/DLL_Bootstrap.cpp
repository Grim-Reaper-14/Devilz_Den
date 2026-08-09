#include "DLL_Bootstrap.hpp"

#include "Runtime_Manager.hpp"

#include <atomic>

namespace Devilz::DLL_Bootstrap
{
namespace
{
std::atomic_bool g_stopRequested{false};
std::atomic_bool g_running{false};
HMODULE g_module = nullptr;

DWORD WINAPI BootstrapThread(void* parameter)
{
    auto* module = static_cast<HMODULE>(parameter);
    Runtime_Manager runtime;

    if (!runtime.Start()) {
        g_running.store(false);
        ::FreeLibraryAndExitThread(module, 1);
    }

    g_running.store(true);

    while (!g_stopRequested.load())
        ::Sleep(50);

    runtime.Stop();
    g_running.store(false);
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
