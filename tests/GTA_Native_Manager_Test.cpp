#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Call_Context.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Manager.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Registry.hpp"

#include <Windows.h>

#include <cstdint>
#include <cstring>
#include <iostream>

namespace
{
using namespace Devilz::Integrations::GTA5_Enhanced;

constexpr std::uint64_t Fingerprint = GTA_Native_Registry::SupportedFingerprint;

enum class HashHandlerMode
{
    StringHash,
    VectorFixup
};

HashHandlerMode g_hashHandlerMode = HashHandlerMode::StringHash;
std::size_t g_voidInvocationCount = 0;

void FakeProbeHandler(GTA_Native_Call_Context*) {}

void FakeGetGameTimer(GTA_Native_Call_Context* context)
{
    const int value = 424242;
    std::memcpy(context->returnValue, &value, sizeof(value));
}

void FakeGetHashKey(GTA_Native_Call_Context* context)
{
    if (!context || context->argumentCount != 1)
        return;

    if (g_hashHandlerMode == HashHandlerMode::StringHash) {
        const char* text = nullptr;
        std::memcpy(&text, context->arguments, sizeof(text));
        const std::uint32_t value = text && std::strcmp(text, "devilz_den") == 0
            ? 0xD3711A5Eu
            : 0u;
        std::memcpy(context->returnValue, &value, sizeof(value));
        return;
    }

    GTA_Native_Script_Vector* target = nullptr;
    std::memcpy(&target, context->arguments, sizeof(target));
    if (!target)
        return;

    context->vectorReferenceTargets[0] = target;
    context->vectorReferenceSources[0] = {1.25F, 2.5F, 3.75F};
    context->vectorReferenceCount = 1;
    ++g_voidInvocationCount;
}

void GoodBootstrap(GTA_Native_Program_Bootstrap* program)
{
    if (!program || !program->nativeEntrypoints)
        return;

    const auto timer = GTA_Native_Registry::Find(Fingerprint, GTA_Native_Id::GetGameTimer);
    const auto hashKey = GTA_Native_Registry::Find(Fingerprint, GTA_Native_Id::GetHashKey);
    if (!timer || !hashKey)
        return;

    for (std::uint32_t i = 0; i < program->nativeCount; ++i) {
        const auto requestedHash = static_cast<GTA_Native_Hash>(
            reinterpret_cast<std::uintptr_t>(program->nativeEntrypoints[i]));

        if (requestedHash == timer->enhancedHash)
            program->nativeEntrypoints[i] = &FakeGetGameTimer;
        else if (requestedHash == hashKey->enhancedHash)
            program->nativeEntrypoints[i] = &FakeGetHashKey;
        else
            program->nativeEntrypoints[i] = &FakeProbeHandler;
    }
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

    GTA_Native_Script_Vector vectorValue{};
    vectorValue.x = 10.0F;
    vectorValue.y = 20.0F;
    vectorValue.z = 30.0F;
    std::memcpy(frame.Context().returnValue, &vectorValue, sizeof(vectorValue));
    const auto copiedVector = frame.Return<GTA_Native_Script_Vector>();
    if (copiedVector.x != 10.0F || copiedVector.y != 20.0F || copiedVector.z != 30.0F) {
        std::cerr << "Native vector return transport failed\n";
        return false;
    }

    return true;
}

bool TestRegistry()
{
    const auto timer = GTA_Native_Registry::Find(Fingerprint, GTA_Native_Id::GetGameTimer);
    const auto hashKey = GTA_Native_Registry::Find(Fingerprint, GTA_Native_Id::GetHashKey);
    const auto playerPed = GTA_Native_Registry::Find(Fingerprint, GTA_Native_Id::PlayerPedId);
    const auto invincible = GTA_Native_Registry::Find(Fingerprint, GTA_Native_Id::SetEntityInvincible);
    const auto waypoint = GTA_Native_Registry::Find(Fingerprint, GTA_Native_Id::IsWaypointActive);
    const auto waterHeight = GTA_Native_Registry::Find(Fingerprint, GTA_Native_Id::GetWaterHeight);
    const auto approxHeight = GTA_Native_Registry::Find(Fingerprint, GTA_Native_Id::GetApproxHeightForPoint);
    const auto setCoords = GTA_Native_Registry::Find(Fingerprint, GTA_Native_Id::SetEntityCoordsNoOffset);
    const auto disableControls = GTA_Native_Registry::Find(Fingerprint, GTA_Native_Id::DisableAllControlActions);

    if (!timer || timer->originalHash != 0x9CD27B0045628463ULL ||
        timer->enhancedHash != 0x1DD05E817C89C737ULL) {
        std::cerr << "GET_GAME_TIMER registry mapping is incorrect\n";
        return false;
    }

    if (!hashKey || hashKey->originalHash != 0xD24D37CC275948CCULL ||
        hashKey->enhancedHash != 0x70E57E9927B6BA58ULL) {
        std::cerr << "GET_HASH_KEY registry mapping is incorrect\n";
        return false;
    }

    if (!playerPed || playerPed->enhancedHash != 0x4A8C381C258A124DULL ||
        !invincible || invincible->enhancedHash != 0x935364B4448CD584ULL ||
        !waypoint || waypoint->enhancedHash != 0x02213DC34A224533ULL ||
        !waterHeight || waterHeight->enhancedHash != 0xF85C2BE613AD7903ULL ||
        !approxHeight || approxHeight->enhancedHash != 0x54D01A0F98391D5BULL ||
        !setCoords || setCoords->enhancedHash != 0x62C438C53BB57AFDULL ||
        !disableControls || disableControls->enhancedHash != 0xD4510218399ED105ULL) {
        std::cerr << "Gameplay native registry mapping is incorrect\n";
        return false;
    }

    if (GTA_Native_Registry::Find(0xDEADBEEFULL, GTA_Native_Id::GetGameTimer)) {
        std::cerr << "Unsupported native fingerprint was accepted\n";
        return false;
    }

    return true;
}

bool TestGoodBootstrapAndInvocation()
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
        imageSize,
        Fingerprint);

    constexpr std::size_t expectedHandlers =
        GTA_Native_Manager::BootstrapProbeHashes.size() + GTA_Native_Registry::NamedIds.size();
    if (!status.ready || !manager.Ready() ||
        status.cachedHandlers != expectedHandlers ||
        manager.CachedHandlerCount() != expectedHandlers) {
        std::cerr << "Valid native bootstrap cache was rejected: " << status.detail << '\n';
        return false;
    }

    for (const auto id : GTA_Native_Registry::NamedIds) {
        if (!manager.Find(id)) {
            std::cerr << "Named gameplay native handler could not be found\n";
            return false;
        }
    }

    const auto timer = manager.Invoke<int>(GTA_Native_Id::GetGameTimer);
    if (!timer || *timer != 424242) {
        std::cerr << "GET_GAME_TIMER invocation return transport failed\n";
        return false;
    }

    g_hashHandlerMode = HashHandlerMode::StringHash;
    const char* text = "devilz_den";
    const auto hash = manager.Invoke<std::uint32_t>(GTA_Native_Id::GetHashKey, text);
    if (!hash || *hash != 0xD3711A5Eu) {
        std::cerr << "Native invocation argument transport failed\n";
        return false;
    }

    g_hashHandlerMode = HashHandlerMode::VectorFixup;
    g_voidInvocationCount = 0;
    GTA_Native_Script_Vector target{};
    if (!manager.Invoke<void>(GTA_Native_Id::GetHashKey, &target) ||
        g_voidInvocationCount != 1 ||
        target.x != 1.25F || target.y != 2.5F || target.z != 3.75F) {
        std::cerr << "Native void invocation or vector fixup failed\n";
        return false;
    }

    const auto missing = manager.Invoke<int>(static_cast<GTA_Native_Id>(0xFF));
    if (missing) {
        std::cerr << "Unknown named native invocation did not fail closed\n";
        return false;
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
        imageSize,
        Fingerprint);

    if (status.ready || manager.Ready() || manager.CachedHandlerCount() != 0) {
        std::cerr << "Invalid native handler was accepted\n";
        return false;
    }

    return true;
}

bool TestUnsupportedFingerprintFailsClosed()
{
    std::uintptr_t imageBase = 0;
    std::size_t imageSize = 0;
    if (!CurrentImage(imageBase, imageSize))
        return false;

    GTA_Native_Manager manager;
    const auto status = manager.Initialize(
        reinterpret_cast<std::uintptr_t>(&GoodBootstrap),
        imageBase,
        imageSize,
        0xDEADBEEFULL);

    if (status.ready || manager.Ready() || manager.CachedHandlerCount() != 0) {
        std::cerr << "Unsupported fingerprint initialized the native manager\n";
        return false;
    }

    return true;
}
}

int main()
{
    if (!TestCallContextLayoutAndFrame() ||
        !TestRegistry() ||
        !TestGoodBootstrapAndInvocation() ||
        !TestBadBootstrapFailsClosed() ||
        !TestUnsupportedFingerprintFailsClosed()) {
        return 1;
    }

    std::cout << "GTA native manager tests passed\n";
    return 0;
}
