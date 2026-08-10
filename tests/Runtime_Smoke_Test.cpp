#include <Windows.h>

#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>

namespace
{
using IsRunningFn = BOOL (*)() noexcept;
using RequestUnloadFn = void (*)() noexcept;
}

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::cerr << "Usage: Devilz_Den_Smoke_Test <path-to-Devilz_Den.dll>\n";
        return 2;
    }

    const std::filesystem::path dllPath = std::filesystem::absolute(argv[1]);
    const std::wstring dllPathWide = dllPath.wstring();
    const std::wstring moduleName = dllPath.filename().wstring();

    HMODULE module = ::LoadLibraryW(dllPathWide.c_str());
    if (!module) {
        std::cerr << "LoadLibraryW failed with Win32 error " << ::GetLastError() << '\n';
        return 3;
    }

    const auto isRunning = reinterpret_cast<IsRunningFn>(
        ::GetProcAddress(module, "Devilz_Den_IsRunning"));
    const auto requestUnload = reinterpret_cast<RequestUnloadFn>(
        ::GetProcAddress(module, "Devilz_Den_RequestUnload"));

    if (!isRunning || !requestUnload) {
        std::cerr << "Required Devilz_Den exports are missing\n";
        return 4;
    }

    bool started = false;
    for (int attempt = 0; attempt < 400; ++attempt) {
        if (!::GetModuleHandleW(moduleName.c_str())) {
            std::cerr << "Devilz_Den.dll unloaded during bootstrap\n";
            return 5;
        }

        if (isRunning()) {
            started = true;
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    if (!started) {
        std::cerr << "Devilz_Den runtime did not report running\n";
        return 6;
    }

    requestUnload();

    for (int attempt = 0; attempt < 400; ++attempt) {
        if (!::GetModuleHandleW(moduleName.c_str())) {
            std::cout << "Devilz_Den DLL bootstrap smoke test passed\n";
            return 0;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    std::cerr << "Devilz_Den.dll did not unload cleanly\n";
    return 7;
}
