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
    // The first four are the original bootstrap probes. The remaining hashes are
    // read-only vehicle state helpers used by Devils Forge to capture the car
    // exactly as GTA/LSC currently has it configured.
    static constexpr std::array<GTA_Native_Hash, 17> BootstrapProbeHashes{
        0x4EDE34FBADD967A6ULL,
        0xE81651AD79516E48ULL,
        0xB8BA7F44DF1575E1ULL,
        0xEB1C67C3A5333A92ULL,
        0xFF4B16F297D9CB3EULL, // GET_VEHICLE_COLOURS
        0x741D9B0685E67684ULL, // GET_VEHICLE_EXTRA_COLOURS
        0xB8090FC59766A88CULL, // GET_VEHICLE_MOD_COLOR_1
        0x07AE5F5D5A7D0936ULL, // GET_VEHICLE_MOD_COLOR_2
        0xA9D64A14804D119BULL, // GET_IS_VEHICLE_PRIMARY_COLOUR_CUSTOM
        0xD9B9D4D1CCED7CA6ULL, // GET_VEHICLE_CUSTOM_PRIMARY_COLOUR
        0x2C0B2BB7913E8DBAULL, // GET_IS_VEHICLE_SECONDARY_COLOUR_CUSTOM
        0x04434FA56DED5500ULL, // GET_VEHICLE_CUSTOM_SECONDARY_COLOUR
        0xEFDD8C5443F6C9E4ULL, // GET_VEHICLE_MOD_VARIATION
        0x1D5A665629D417A7ULL, // IS_TOGGLE_MOD_ON
        0xCA7159F2C5FF745AULL, // GET_VEHICLE_NUMBER_PLATE_TEXT
        0x963D9A7202C06F65ULL, // CLEAR_VEHICLE_CUSTOM_PRIMARY_COLOUR
        0x588D8FDC61F7CFADULL  // CLEAR_VEHICLE_CUSTOM_SECONDARY_COLOUR
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
        return InvokeHandler<Ret>(Find(id), std::forward<Args>(args)...);
    }

    template <typename Ret, typename... Args>
    [[nodiscard]] auto InvokeHash(GTA_Native_Hash hash, Args&&... args) noexcept
        -> typename GTA_Native_Invoke_Result<Ret>::Type
    {
        return InvokeHandler<Ret>(Find(hash), std::forward<Args>(args)...);
    }

private:
    template <typename Ret, typename... Args>
    [[nodiscard]] auto InvokeHandler(GTA_Native_Handler handler, Args&&... args) noexcept
        -> typename GTA_Native_Invoke_Result<Ret>::Type
    {
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

    [[nodiscard]] static bool IsExecutableImageAddress(
        std::uintptr_t address,
        std::uintptr_t moduleBase,
        std::size_t moduleSize) noexcept;

    bool m_ready = false;
    std::uint64_t m_fingerprint = 0;
    std::unordered_map<GTA_Native_Hash, GTA_Native_Handler> m_handlers;
};
}
