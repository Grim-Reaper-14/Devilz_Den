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
    // Validated Enhanced handlers which do not need a public named-id yet live
    // in this bootstrap cache. Vehicle Forge uses the bulk of these; Self and
    // Weapons also use the final handlers for movement, explosive-ammo guards,
    // Off The Radar network time, and cutscene skipping.
    static constexpr std::array<GTA_Native_Hash, 47> BootstrapProbeHashes{
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
        0x588D8FDC61F7CFADULL, // CLEAR_VEHICLE_CUSTOM_SECONDARY_COLOUR
        0x90E3EAFF8AAA1A42ULL, // GET_NUM_MOD_KITS
        0xE62930EC6FAABCA5ULL, // SET_VEHICLE_NEON_ENABLED
        0xF1B79038130E3C08ULL, // GET_VEHICLE_NEON_ENABLED
        0xEAB8A43F6621850FULL, // SET_VEHICLE_NEON_COLOUR
        0x64FEACF0AD019F1FULL, // GET_VEHICLE_NEON_COLOUR
        0x89D1FDCA3735A1E0ULL, // SET_VEHICLE_XENON_LIGHT_COLOR_INDEX
        0xD6BA8C57BDF9DEB9ULL, // GET_VEHICLE_XENON_LIGHT_COLOR_INDEX
        0xD772F6AA66750D2BULL, // SET_VEHICLE_EXTRA
        0x579FA5568DE0C2A0ULL, // DOES_EXTRA_EXIST
        0x5318DF85BEB6B95FULL, // IS_VEHICLE_EXTRA_TURNED_ON
        0x5DA0536AEAD1FF31ULL, // SET_VEHICLE_TYRE_SMOKE_COLOR
        0x9D35AABAEE206518ULL, // GET_VEHICLE_TYRE_SMOKE_COLOR
        0x439C904840715871ULL, // SET_VEHICLE_TYRES_CAN_BURST
        0xE6BE8A525BA6BD44ULL, // GET_VEHICLE_TYRES_CAN_BURST
        0x519F76A38952BBD0ULL, // SET_DRIFT_TYRES
        0x4497678941C27E46ULL, // GET_DRIFT_TYRES_SET
        0xC0C8E6AAA00F1A58ULL, // SET_VEHICLE_EXTRA_COLOUR_5
        0xE10BD9712D7B0CBFULL, // GET_VEHICLE_EXTRA_COLOUR_5
        0x77B012A683295B6EULL, // SET_VEHICLE_EXTRA_COLOUR_6
        0x4C5611B5008205EBULL, // GET_VEHICLE_EXTRA_COLOUR_6
        0xA1C03303EC67320BULL, // SET_VEHICLE_LIVERY
        0xA089B04A208DBD0BULL, // GET_VEHICLE_LIVERY
        0xBA3ECE95D3094B0FULL, // GET_VEHICLE_LIVERY_COUNT
        0xA52E1AE3848A506BULL, // SET_RUN_SPRINT_MULTIPLIER_FOR_PLAYER
        0x289497A4BA9049E0ULL, // SET_SWIM_MULTIPLIER_FOR_PLAYER
        0xB27B08E34AC92345ULL, // SET_PED_MOVE_RATE_OVERRIDE
        0x11552FA9DCB8E126ULL, // IS_PED_ARMED
        0xB73833BDAAE31047ULL, // IS_PED_PERFORMING_MELEE_ACTION
        0x7E3F74F641EE6B27ULL, // GET_NETWORK_TIME
        0xA7E4AA8D29D3DAC1ULL  // STOP_CUTSCENE_IMMEDIATELY
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
