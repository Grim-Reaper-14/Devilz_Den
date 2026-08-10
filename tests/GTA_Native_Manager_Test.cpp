#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Call_Context.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Manager.hpp"

#include <Windows.h>

#include <cstdint>
#include <iostream>

namespace
{
using namespace Devilz::Integrations::GTA5_Enhanced;

void FakeHandlerA(GTA_Native_Call_Context*) {}
void FakeHandlerB(GTA_Native_Call_Context*) {}

void GoodBootstrap(GTA_Native_Program_Bootstrap* program)
{
    if (!program || !program->nativeEntrypoints)
        return;

    for (std::uint32_t i = 0; i < program->nativeCount; ++i)
        program->nativeEntrypoints[i] = (i % 2 == 0) ? &FakeHandlerA : &FakeHandlerB;
}

void BadBootstrap(GTA_Native_Program_Bootstrap* program)
{
    GoodBootstrap(program);
    if (program && program->nativeEntrypoints && program->nativeCount != 0) {
        program->nativeEntrypoints[0] =
            reinterpret_cast<GTA_Native_Handler>(static_cast<std::uintptr_t>(0x1234));
    }
}

bool CurrentImage(std::uintptr_t& base, std::size_t& size)
{
    const auto module = ::GetModuleHandleW(nullptr);
    if (!module)
        return false;

    base = reinterpret_cast<std::uintptr_t>(module);
    const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
        return false;

    const auto nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE)
        return false;

    size = nt->OptionalHeader.SizeOfImage;
    return size != 0;
}

bool TestCallContextLayoutAndFrame()
{
    GTA_Native_Call_Frame frame;
    if (!frame.Push<std::uint32_t>(0x11223344u) ||
        !frame.Push<float>(3.5F) ||
        frame.Context().argumentCount != 2) {
        std::cerr << "Native call frame argument packing failed\n";
        return false;
    }

    frame.Reset();
    if (frame.Context().argumentCount != 0 || frame.Context().vectorReferenceCount != 0) {
        std::cerr << "Native call frame reset failed\n";
        return false;
    }

    return true;
}

bool TestGoodBootstrap()
{
    std::uintptr_t imageBase = 0;
    std::size_t imageSize = 0;
    if (!CurrentImage(imageBase, imageSize)) {
        std::cerr << "Could not inspect native-manager test image\n";
        return false;
    }

    GTA_Native_Manager manager;
    const auto status = manager.Initialize(
        reinterpret_cast<std::uintptr_t>(&GoodBootstrap),
        imageBase,
        imageSize);

    if (!status.ready || !manager.Ready() ||
        status.cachedHandlers != GTA_Native_Manager::BootstrapProbeHashes.size()) {
        std::cerr << "Valid native bootstrap cache was rejected: " << status.detail << '\n';
        return false;
    }

    for (const auto hash : GTA_Native_Manager::BootstrapProbeHashes) {
        if (!manager.Find(hash)) {
            std::cerr << "Cached native probe handler could not be found\n";
            return false;
        }
    }

    return true;
}

bool TestBadBootstrapFailsClosed()
{
    std::uintptr_t imageBase = 0;
    std::size_t imageSize = 0;
    if (!CurrentImage(imageBase, imageSize))
        return false;

    GTA_Native_Manager manager;
    const auto status = manager.Initialize(
        reinterpret_cast<std::uintptr_t>(&BadBootstrap),
        imageBase,
        imageSize);

    if (status.ready || manager.Ready() || manager.CachedHandlerCount() != 0) {
        std::cerr << "Invalid native handler was accepted\n";
        return false;
    }

    return true;
}
}

int main()
{
    if (!TestCallContextLayoutAndFrame() ||
        !TestGoodBootstrap() ||
        !TestBadBootstrapFailsClosed()) {
        return 1;
    }

    std::cout << "GTA native manager tests passed\n";
    return 0;
}
