#pragma once

#include "GTA_Native_Call_Context.hpp"
#include "GTA_Native_Registry.hpp"
#include "GTA_Native_Types.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace Devilz::Integrations::GTA5_Enhanced
{
struct GTA_Native_Manager_Status
{
    bool ready = false;
    std::size_t requestedHandlers = 0;
    std::size_t cachedHandlers = 0;
    std::string detail;
};

template <typename T>
struct GTA_Native_Invoke_Result
{
    using Type = std::optional<T>;
};

template <>
struct GTA_Native_Invoke_Result<void>
{
    using Type = bool;
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
        std::size_t moduleSize,
        std::uint64_t fingerprint);

    void Reset() noexcept;

    [[nodiscard]] bool Ready() const noexcept { return m_ready; }
    [[nodiscard]] std::uint64_t Fingerprint() const noexcept { return m_fingerprint; }
    [[nodiscard]] std::size_t CachedHandlerCount() const noexcept { return m_handlers.size(); }
    [[nodiscard]] GTA_Native_Handler Find(GTA_Native_Hash hash) const noexcept;
    [[nodiscard]] GTA_Native_Handler Find(GTA_Native_Id id) const noexcept;

    template <typename Ret, typename... Args>
    [[nodiscard]] auto Invoke(GTA_Native_Id id, Args&&... args) noexcept
        -> typename GTA_Native_Invoke_Result<Ret>::Type
    {
        const auto handler = Find(id);
        if (!m_ready || !handler) {
            if constexpr (std::is_void_v<Ret>)
                return false;
            else
                return std::nullopt;
        }

        GTA_Native_Call_Frame frame;
        const bool packed = (frame.Push(std::forward<Args>(args)) && ...);
        if (!packed) {
            if constexpr (std::is_void_v<Ret>)
                return false;
            else
                return std::nullopt;
        }

        handler(&frame.Context());
        frame.FixVectors();

        if constexpr (std::is_void_v<Ret>)
            return true;
        else
            return frame.Return<Ret>();
    }

private:
    [[nodiscard]] static bool IsExecutableImageAddress(
        std::uintptr_t address,
        std::uintptr_t moduleBase,
        std::size_t moduleSize) noexcept;

    bool m_ready = false;
    std::uint64_t m_fingerprint = 0;
    std::unordered_map<GTA_Native_Hash, GTA_Native_Handler> m_handlers;
};
}
