#include "GTA_Business_Extension.hpp"

#include "GTA_Business_State.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Manager.hpp"
#include "Integrations/GTA5_Enhanced/Script/Globals/Script_Global_Manager.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>

namespace Devilz::Integrations::GTA5_Enhanced
{
namespace
{
constexpr std::uint64_t SupportedFingerprint = 0x6A4F97F605B81000ULL;

// GTA Online Enhanced 1.73 / b1158.13.
constexpr std::uint32_t TunablesBase = 262145U;
constexpr std::uint32_t GpbdFmBase = 1845347U;
constexpr std::uint32_t GpbdFmPlayerStride = 884U;
constexpr std::uint32_t PropertyDataOffset = 260U;
constexpr std::uint32_t BusinessHubOffset = 321U;
constexpr std::uint32_t ProductStocksOffset = BusinessHubOffset + 9U;
constexpr std::uint32_t NightclubDataOffset = 364U;
constexpr std::uint32_t NightclubPopularityOffset = NightclubDataOffset + 4U;
constexpr std::uint32_t NightclubSafeOffset = NightclubDataOffset + 5U;

constexpr std::uint32_t EquipmentUpgradeMultiplier = 24047U;
constexpr std::uint32_t ManagementMissionCooldown = 24118U;
constexpr std::uint32_t SellMissionCooldown = 24159U;
constexpr std::uint32_t SpecialOrderSellCooldown = 24160U;
constexpr std::uint32_t PopularityIncomeBase = 23750U;

constexpr auto RefreshInterval = std::chrono::milliseconds(500);

struct NightclubGoodDefinition
{
    GTA_Nightclub_Good good{};
    std::uint32_t liveStockIndex = 0;
    std::uint32_t stockValueOffset = 0;
    std::uint32_t specialOrderValueOffset = 0;
    std::uint32_t maxUnitsOffset = 0;
    std::uint32_t productionTimeOffset = 0;
};

constexpr std::array<NightclubGoodDefinition, GTA_Nightclub_Good_Count> GoodDefinitions{{
    {GTA_Nightclub_Good::Cargo, 0U, 24061U, 24054U, 24068U, 24046U},
    {GTA_Nightclub_Good::SportingGoods, 1U, 24055U, 24048U, 24062U, 24040U},
    {GTA_Nightclub_Good::SouthAmericanImports, 2U, 24056U, 24049U, 24063U, 24041U},
    {GTA_Nightclub_Good::PharmaceuticalResearch, 3U, 24057U, 24050U, 24064U, 24042U},
    {GTA_Nightclub_Good::OrganicProduce, 4U, 24058U, 24051U, 24065U, 24043U},
    {GTA_Nightclub_Good::PrintingAndCopying, 5U, 24059U, 24052U, 24066U, 24044U},
    {GTA_Nightclub_Good::CashCreation, 6U, 24060U, 24053U, 24067U, 24045U}
}};

struct BusinessRuntime
{
    Script_Global_Manager* globals = nullptr;
    std::uint64_t buildFingerprint = 0;
    std::chrono::steady_clock::time_point lastRefresh{};
};

BusinessRuntime g_runtime{};

template <typename T>
bool ReadGlobal(Script_Global_Manager& globals, std::uint32_t index, T& value)
{
    auto result = globals.Get(index).Read<T>();
    if (!result)
        return false;

    value = result.Value();
    return true;
}

std::uint32_t PlayerPropertyData(int player) noexcept
{
    return GpbdFmBase +
        1U +
        static_cast<std::uint32_t>(player) * GpbdFmPlayerStride +
        PropertyDataOffset;
}

GTA_Nightclub_Snapshot BuildNightclubSnapshot(GTA_Native_Manager& natives)
{
    GTA_Nightclub_Snapshot snapshot{};
    auto* globals = g_runtime.globals;

    if (!globals || !globals->Ready()) {
        snapshot.detail = "Script globals are not configured.";
        return snapshot;
    }

    if (g_runtime.buildFingerprint != SupportedFingerprint) {
        snapshot.detail = "Nightclub globals are not registered for this GTA build.";
        return snapshot;
    }

    const auto player = natives.Invoke<int>(GTA_Native_Id::PlayerId);
    if (!player || *player < 0 || *player >= 32) {
        snapshot.detail = "Local player index is unavailable.";
        return snapshot;
    }

    snapshot.playerIndex = *player;
    const auto property = PlayerPropertyData(*player);

    bool liveOk = true;
    liveOk &= ReadGlobal(*globals, property + NightclubDataOffset, snapshot.nightclubIndex);
    liveOk &= ReadGlobal(*globals, property + NightclubPopularityOffset, snapshot.popularity);
    liveOk &= ReadGlobal(*globals, property + NightclubSafeOffset, snapshot.safeCash);

    if (!liveOk) {
        snapshot.detail = "Nightclub live state could not be read.";
        return snapshot;
    }

    snapshot.owned = snapshot.nightclubIndex != 0;

    ReadGlobal(
        *globals,
        TunablesBase + EquipmentUpgradeMultiplier,
        snapshot.equipmentUpgradeMultiplier);
    ReadGlobal(
        *globals,
        TunablesBase + ManagementMissionCooldown,
        snapshot.managementMissionCooldownMs);
    ReadGlobal(
        *globals,
        TunablesBase + SellMissionCooldown,
        snapshot.sellMissionCooldownMs);
    ReadGlobal(
        *globals,
        TunablesBase + SpecialOrderSellCooldown,
        snapshot.specialOrderSellCooldownMs);

    for (std::size_t i = 0; i < snapshot.popularityIncome.size(); ++i) {
        ReadGlobal(
            *globals,
            TunablesBase + PopularityIncomeBase + static_cast<std::uint32_t>(i),
            snapshot.popularityIncome[i]);
    }

    for (const auto& definition : GoodDefinitions) {
        auto& good = snapshot.goods[static_cast<std::size_t>(definition.good)];
        bool goodOk = true;

        goodOk &= ReadGlobal(
            *globals,
            property + ProductStocksOffset + definition.liveStockIndex,
            good.units);
        goodOk &= ReadGlobal(
            *globals,
            TunablesBase + definition.maxUnitsOffset,
            good.maxUnits);
        goodOk &= ReadGlobal(
            *globals,
            TunablesBase + definition.stockValueOffset,
            good.unitValue);
        goodOk &= ReadGlobal(
            *globals,
            TunablesBase + definition.specialOrderValueOffset,
            good.specialOrderUnitValue);
        goodOk &= ReadGlobal(
            *globals,
            TunablesBase + definition.productionTimeOffset,
            good.productionTimeMs);

        good.available = goodOk;
    }

    snapshot.runtimeReady = true;
    snapshot.detail = snapshot.owned
        ? "Nightclub business data ready."
        : "Nightclub runtime ready; no owned Nightclub detected.";
    return snapshot;
}
}

void ConfigureBusinessExtension(
    Script_Global_Manager* globals,
    std::uint64_t buildFingerprint) noexcept
{
    g_runtime.globals = globals;
    g_runtime.buildFingerprint = buildFingerprint;
    g_runtime.lastRefresh = {};
    GTA_Business_State::Instance().Reset();
}

void ResetBusinessExtension() noexcept
{
    g_runtime = {};
    GTA_Business_State::Instance().Reset();
}

void TickBusinessExtension(GTA_Native_Manager& natives) noexcept
{
    if (!g_runtime.globals)
        return;

    const auto now = std::chrono::steady_clock::now();
    if (g_runtime.lastRefresh != std::chrono::steady_clock::time_point{} &&
        now - g_runtime.lastRefresh < RefreshInterval) {
        return;
    }

    g_runtime.lastRefresh = now;
    GTA_Business_State::Instance().PublishNightclub(BuildNightclubSnapshot(natives));
}
}
