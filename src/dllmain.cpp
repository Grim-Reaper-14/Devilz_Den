#include "Runtime/DLL_Bootstrap.hpp"

#include <Windows.h>

extern "C" __declspec(dllexport) void Devilz_Den_RequestUnload() noexcept
{
    Devilz::DLL_Bootstrap::RequestUnload();
}

extern "C" __declspec(dllexport) BOOL Devilz_Den_IsRunning() noexcept
{
    return Devilz::DLL_Bootstrap::Running() ? TRUE : FALSE;
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved)
{
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        ::DisableThreadLibraryCalls(module);
        if (!Devilz::DLL_Bootstrap::Attach(module))
            return FALSE;
        break;

    case DLL_PROCESS_DETACH:
        if (reserved == nullptr)
            Devilz::DLL_Bootstrap::RequestUnload();
        break;

    default:
        break;
    }

    return TRUE;
}
