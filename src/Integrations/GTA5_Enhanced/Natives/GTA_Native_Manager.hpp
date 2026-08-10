#pragma once

#include "GTA_Native_Types.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace Devilz::Integrations::GTA5_Enhanced
{
struct GTA_Native_Manager_Status
{
    bool ready = false;
    std::size_t requestedHandlers = 0;
    std::size_t cachedHandlers = 0;
    std::string detail;
};

class GTA_Native_Manager final
{
public:
    static constexpr std::array<GTA_Native_Hash, 4> BootstrapProbeHashes{
        0x4EDE34FBADD967A6ULL,
        0xE81651AD79516E48ULL,
        0xB8BA7F44DF1575E1ULL,
        0xEB1C67C3A5333A92ULL
    };

    [[nodiscard]] GTA_Native_Manager_Status Initialize(
        std::uintptr_t initNativeTablesAddress,
        std::uintptr_t moduleBase,
        std::size_t moduleSize);

    void Reset() noexcept;

    [[nodiscard]] bool Ready() const noexcept { return m_ready; }
    [[nodiscard]] std::size_t CachedHandlerCount() const noexcept { return m_handlers.size(); }
    [[nodiscard]] GTA_Native_Handler Find(GTA_Native_Hash hash) const noexcept;

private:
    [[nodiscard]] static bool IsExecutableImageAddress(
        std::uintptr_t address,
        std::uintptr_t moduleBase,
        std::size_t moduleSize) noexcept;

    bool m_ready = false;
    std::unordered_map<GTA_Native_Hash, GTA_Native_Handler> m_handlers;
};
}
