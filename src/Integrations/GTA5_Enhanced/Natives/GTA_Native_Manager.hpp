#pragma once

#include "GTA_Native_Call_Context.hpp"
#include "GTA_Native_Registry.hpp"
#include "GTA_Native_Types.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Business_Extension.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Outfit_Editor_Extension.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Packed_Stats_State.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Ped_Control.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Self_Online_Extension.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Self_Utility_Extension.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Stats_Extension.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Teleport_Extension.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Vehicle_Garage_Save.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_World_Environment_Extension.hpp"

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
    static constexpr std::array<GTA_Native_Hash, 86> BootstrapProbeHashes{
        0x4EDE34FBADD967A6ULL,
        0xE81651AD79516E48ULL,
        0xB8BA7F44DF1575E1ULL,
        0xEB1C67C3A5333A92ULL,
        0xFF4B16F297D9CB3EULL,
        0x741D9B0685E67684ULL,
        0xB8090FC59766A88CULL,
        0x07AE5F5D5A7D0936ULL,
        0xA9D64A14804D119BULL,
        0xD9B9D4D1CCED7CA6ULL,
        0x2C0B2BB7913E8DBAULL,
        0x04434FA56DED5500ULL,
        0xEFDD8C5443F6C9E4ULL,
        0x1D5A665629D417A7ULL,
        0xCA7159F2C5FF745AULL,
        0x963D9A7202C06F65ULL,
        0x588D8FDC61F7CFADULL,
        0x90E3EAFF8AAA1A42ULL,
        0xE62930EC6FAABCA5ULL,
        0xF1B79038130E3C08ULL,
        0xEAB8A43F6621850FULL,
        0x64FEACF0AD019F1FULL,
        0x89D1FDCA3735A1E0ULL,
        0xD6BA8C57BDF9DEB9ULL,
        0xD772F6AA66750D2BULL,
        0x579FA5568DE0C2A0ULL,
        0x5318DF85BEB6B95FULL,
        0x5DA0536AEAD1FF31ULL,
        0x9D35AABAEE206518ULL,
        0x439C904840715871ULL,
        0xE6BE8A525BA6BD44ULL,
        0x519F76A38952BBD0ULL,
        0x4497678941C27E46ULL,
        0xC0C8E6AAA00F1A58ULL,
        0xE10BD9712D7B0CBFULL,
        0x77B012A683295B6EULL,
        0x4C5611B5008205EBULL,
        0xA1C03303EC67320BULL,
        0xA089B04A208DBD0BULL,
        0xBA3ECE95D3094B0FULL,
        0xA52E1AE3848A506BULL,
        0x289497A4BA9049E0ULL,
        0xB27B08E34AC92345ULL,
        0x11552FA9DCB8E126ULL,
        0xB73833BDAAE31047ULL,
        0x7E3F74F641EE6B27ULL,
        0x71A6F836422FDD2BULL,
        0xA7E4AA8D29D3DAC1ULL,
        0xAFD3BC0F6EBB5474ULL,
        0x99599AE2C0FDB2A1ULL,
        0x88791F880F624022ULL,
        0x58A3B74F26D2B532ULL,
        0xD25E9BDC14A0B649ULL,
        0x501EBB0523078750ULL,
        0x1B32E388988DD296ULL,
        0x1E37AEC038A241A3ULL,
        0x92EBF838856DCF63ULL,
        0xC2BF1F6F84E31EB2ULL,
        0xD33BCB9F50C1E588ULL,
        0xE3D5A2DE522F29C1ULL,
        0x5F5FDED45A3345C9ULL,
        0xD1C578C204015E1FULL,
        0xC0120BBCC298EA2FULL,
        0x1A4EFE92822E3123ULL,
        0xD6AED6BFCC58AF7FULL,
        0x8401C77F508D70FDULL,
        0xDAF263B0E792EAECULL,
        0xB204F40D393426B6ULL,
        0x4D0F04723A52D0E9ULL,
        0x0DC23FA727759F9FULL,
        0x1D77F90D87ACD2BAULL,
        0x7F08C4791E6D6969ULL,
        0x09397806857F5DFBULL,
        0xDF7F16323520B858ULL,
        0x2F0966A034F5ADC6ULL,
        0xF249567F2E83E093ULL,
        0xCEA81DACD6DA3ADBULL,
        0x1164A75E490C27B6ULL,
        0x4F8678C02360C3D2ULL,
        0xF1D0B0CE940F620DULL,
        0x1A43F9BE4B6AAB67ULL,
        0xD69CE161FE614531ULL,
        0xA6D3C21763E25496ULL,
        0x03CFFD51CE515454ULL,
        0xA595AA1819B05EA0ULL,
        0x0F575D68F532124CULL
    };

    [[nodiscard]] GTA_Native_Manager_Status Initialize(std::uintptr_t initNativeTablesAddress, std::uintptr_t moduleBase, std::size_t moduleSize, std::uint64_t fingerprint);
    void Reset() noexcept;
    [[nodiscard]] bool Ready() const noexcept { return m_ready; }
    [[nodiscard]] std::uint64_t Fingerprint() const noexcept { return m_fingerprint; }
    [[nodiscard]] std::size_t CachedHandlerCount() const noexcept { return m_handlers.size(); }
    [[nodiscard]] GTA_Native_Handler Find(GTA_Native_Hash hash) const noexcept;
    [[nodiscard]] GTA_Native_Handler Find(GTA_Native_Id id) const noexcept;

    template <typename Ret, typename... Args>
    [[nodiscard]] auto Invoke(GTA_Native_Id id, Args&&... args) noexcept -> typename GTA_Native_Invoke_Result<Ret>::Type
    {
        return InvokeHandler<Ret>(Find(id), std::forward<Args>(args)...);
    }

    template <typename Ret, typename... Args>
    [[nodiscard]] auto InvokeHash(GTA_Native_Hash hash, Args&&... args) noexcept -> typename GTA_Native_Invoke_Result<Ret>::Type
    {
        return InvokeHandler<Ret>(Find(hash), std::forward<Args>(args)...);
    }

    template <typename Ret, typename... Args>
    [[nodiscard]] auto InvokeDirectHash(GTA_Native_Hash hash, Args&&... args) noexcept -> typename GTA_Native_Invoke_Result<Ret>::Type
    {
        return InvokeHandler<Ret>(Find(hash), std::forward<Args>(args)...);
    }

    template <typename Ret, typename... Args>
    [[nodiscard]] auto InvokeOptionalHash(GTA_Native_Hash hash, Args&&... args) noexcept -> typename GTA_Native_Invoke_Result<Ret>::Type
    {
        return InvokeHandler<Ret>(FindOptional(hash), std::forward<Args>(args)...);
    }

private:
    template <typename Ret, typename... Args>
    [[nodiscard]] auto InvokeHandler(GTA_Native_Handler handler, Args&&... args) noexcept -> typename GTA_Native_Invoke_Result<Ret>::Type
    {
        if (!m_ready || !handler) {
            if constexpr (std::is_void_v<Ret>) return false;
            else return std::nullopt;
        }
        GTA_Native_Call_Frame frame;
        const bool packed = (frame.Push(std::forward<Args>(args)) && ...);
        if (!packed) {
            if constexpr (std::is_void_v<Ret>) return false;
            else return std::nullopt;
        }
        handler(&frame.Context());
        frame.FixVectors();
        if constexpr (std::is_void_v<Ret>) return true;
        else return frame.Return<Ret>();
    }

    void DrainPackedStatsQueue() noexcept
    {
        constexpr GTA_Native_Hash GetPackedStatBoolCode = 0xA6D3C21763E25496ULL;
        constexpr GTA_Native_Hash GetPackedStatIntCode = 0x03CFFD51CE515454ULL;
        constexpr GTA_Native_Hash SetPackedStatBoolCode = 0xA595AA1819B05EA0ULL;
        constexpr GTA_Native_Hash SetPackedStatIntCode = 0x0F575D68F532124CULL;
        constexpr std::size_t CommandsPerTick = 8;
        auto& state = GTA_Packed_Stats_State::Instance();
        for (std::size_t processed = 0; processed < CommandsPerTick; ++processed) {
            GTA_Packed_Stats_State::Command command{};
            if (!state.Consume(command)) break;
            if (command.kind == GTA_Packed_Stats_State::Command_Kind::Read) {
                if (command.valueType == GTA_Packed_Stat_Value_Type::Bool) {
                    const auto value = InvokeHandler<bool>(Find(GetPackedStatBoolCode), command.index, -1);
                    state.Complete(command, value.has_value(), value.value_or(false) ? 1 : 0);
                } else {
                    const auto value = InvokeHandler<std::int32_t>(Find(GetPackedStatIntCode), command.index, -1);
                    state.Complete(command, value.has_value(), value.value_or(0));
                }
                continue;
            }
            const bool success = command.valueType == GTA_Packed_Stat_Value_Type::Bool
                ? InvokeHandler<void>(Find(SetPackedStatBoolCode), command.index, command.value != 0, -1)
                : InvokeHandler<void>(Find(SetPackedStatIntCode), command.index, command.value, -1);
            state.Complete(command, success, command.value);
        }
    }

    [[nodiscard]] static bool IsExecutableImageAddress(std::uintptr_t address, std::uintptr_t moduleBase, std::size_t moduleSize) noexcept;
    [[nodiscard]] GTA_Native_Handler FindOptional(GTA_Native_Hash hash) const noexcept;

    bool m_ready = false;
    std::uint64_t m_fingerprint = 0;
    std::unordered_map<GTA_Native_Hash, GTA_Native_Handler> m_handlers;
    std::unordered_map<GTA_Native_Hash, GTA_Native_Handler> m_optionalHandlers;
};
}
