#include "GTA_Native_Manager.hpp"

#include <Windows.h>

#include <algorithm>
#include <array>
#include <limits>

namespace Devilz::Integrations::GTA5_Enhanced
{
namespace
{
bool IsExecutableProtection(DWORD protection) noexcept
{
    const auto base = protection & 0xFFu;
    return base == PAGE_EXECUTE ||
           base == PAGE_EXECUTE_READ ||
           base == PAGE_EXECUTE_READWRITE ||
           base == PAGE_EXECUTE_WRITECOPY;
}
}

GTA_Native_Manager_Status GTA_Native_Manager::Initialize(
    std::uintptr_t initNativeTablesAddress,
    std::uintptr_t moduleBase,
    std::size_t moduleSize,
    std::uint64_t fingerprint)
{
    Reset();

    GTA_Native_Manager_Status status{};

    if (initNativeTablesAddress == 0 || moduleBase == 0 || moduleSize == 0 || fingerprint == 0) {
        status.detail = "Native bootstrap prerequisites are incomplete";
        return status;
    }

    if (!IsExecutableImageAddress(initNativeTablesAddress, moduleBase, moduleSize)) {
        status.detail = "InitNativeTables is not an executable GTA image address";
        return status;
    }

    constexpr std::size_t namedNativeCount = GTA_Native_Registry::NamedIds.size();
    constexpr std::size_t requestedHandlerCount = BootstrapProbeHashes.size() + namedNativeCount;
    std::array<GTA_Native_Hash, requestedHandlerCount> requestedHashes{};
    std::copy(BootstrapProbeHashes.begin(), BootstrapProbeHashes.end(), requestedHashes.begin());

    for (std::size_t i = 0; i < GTA_Native_Registry::NamedIds.size(); ++i) {
        const auto definition = GTA_Native_Registry::Find(fingerprint, GTA_Native_Registry::NamedIds[i]);
        if (!definition) {
            status.detail = "No complete native registry is available for this GTA build fingerprint";
            return status;
        }
        requestedHashes[BootstrapProbeHashes.size() + i] = definition->enhancedHash;
    }

    status.requestedHandlers = requestedHashes.size();

    std::array<GTA_Native_Handler, requestedHandlerCount> entries{};
    for (std::size_t i = 0; i < requestedHashes.size(); ++i) {
        entries[i] = reinterpret_cast<GTA_Native_Handler>(
            static_cast<std::uintptr_t>(requestedHashes[i]));
    }

    GTA_Native_Program_Bootstrap program{};
    program.nativeCount = static_cast<std::uint32_t>(entries.size());
    program.nativeEntrypoints = entries.data();

    const auto initialize = reinterpret_cast<GTA_Init_Native_Tables>(initNativeTablesAddress);
    initialize(&program);

    for (std::size_t i = 0; i < entries.size(); ++i) {
        const auto handlerAddress = reinterpret_cast<std::uintptr_t>(entries[i]);
        if (handlerAddress == 0 ||
            !IsExecutableImageAddress(handlerAddress, moduleBase, moduleSize)) {
            status.cachedHandlers = m_handlers.size();
            status.detail = "Native bootstrap returned a handler outside executable GTA image memory";
            Reset();
            return status;
        }

        m_handlers.insert_or_assign(requestedHashes[i], entries[i]);
    }

    m_ready = m_handlers.size() == requestedHashes.size();
    if (m_ready)
        m_fingerprint = fingerprint;

    status.ready = m_ready;
    status.cachedHandlers = m_handlers.size();
    status.detail = m_ready
        ? "Enhanced native bootstrap resolved and validated probe and gameplay native handlers"
        : "Enhanced native bootstrap did not populate the complete handler set";
    return status;
}

void GTA_Native_Manager::Reset() noexcept
{
    if (m_ready) {
        SetSelfSpecialAbilities(false);
        SetSelfNoIdleKick(false);
        TickSelfUtilityExtension(*this);
    }
    ResetSelfUtilityExtension();

    m_ready = false;
    m_fingerprint = 0;
    m_handlers.clear();
}

GTA_Native_Handler GTA_Native_Manager::Find(GTA_Native_Hash hash) const noexcept
{
    const auto it = m_handlers.find(hash);
    return it == m_handlers.end() ? nullptr : it->second;
}

GTA_Native_Handler GTA_Native_Manager::Find(GTA_Native_Id id) const noexcept
{
    const auto definition = GTA_Native_Registry::Find(m_fingerprint, id);
    return definition ? Find(definition->enhancedHash) : nullptr;
}

bool GTA_Native_Manager::IsExecutableImageAddress(
    std::uintptr_t address,
    std::uintptr_t moduleBase,
    std::size_t moduleSize) noexcept
{
    if (address < moduleBase ||
        moduleBase > (std::numeric_limits<std::uintptr_t>::max)() - moduleSize ||
        address >= moduleBase + moduleSize) {
        return false;
    }

    MEMORY_BASIC_INFORMATION memory{};
    if (::VirtualQuery(reinterpret_cast<const void*>(address), &memory, sizeof(memory)) == 0)
        return false;

    if (memory.State != MEM_COMMIT || memory.Type != MEM_IMAGE)
        return false;

    if ((memory.Protect & PAGE_GUARD) != 0 || (memory.Protect & PAGE_NOACCESS) != 0)
        return false;

    return IsExecutableProtection(memory.Protect);
}
}
