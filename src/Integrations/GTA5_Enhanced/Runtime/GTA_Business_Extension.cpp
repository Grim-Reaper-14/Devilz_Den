#include "GTA_Business_Extension.hpp"

#include "GTA_Business_State.hpp"
#include "Integrations/GTA5_Enhanced/Natives/GTA_Native_Manager.hpp"
#include "Integrations/GTA5_Enhanced/Script/Globals/Script_Global_Manager.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>

namespace Devilz::Integrations::GTA5_Enhanced
{
namespace
{
constexpr std::uint64_t SupportedFingerprint = 0x6A4F97F605B81000ULL;

// GTA Online Enhanced 1.73 / b1158.13.
constexpr std::uint32_t TunablesBase = 262145U;
constexpr std::uint32_t GpbdFmBase = 1845347U;
constexpr std::uint32_t GpbdFmPlayerStride = 884U;
constexpr std::uint32_t GpbdFm3Base = 1893070U;
constexpr std::uint32_t GpbdFm3PlayerStride = 615U;
// Global_1673820 + 1..7: five MC factory slots, Bunker, then Acid Lab.
constexpr std::uint32_t FreemodeBusinessBase = 1673820U;
constexpr int InstantResupplyTrigger = 1;
constexpr std::uint32_t PropertyDataOffset = 260U;
constexpr std::uint32_t BusinessHubOffset = 321U;
constexpr std::uint32_t ProductStocksOffset = BusinessHubOffset + 9U;
constexpr std::uint32_t NightclubDataOffset = 364U;
constexpr std::uint32_t NightclubPopularityOffset = NightclubDataOffset + 4U;
constexpr std::uint32_t NightclubSafeOffset = NightclubDataOffset + 5U;
constexpr std::uint32_t NightclubEntryCostOffset = NightclubDataOffset + 6U;

// GPBD_FM entry offsets verified against the b1158.13 884-slot layout.
constexpr std::uint32_t NightclubSaleOffset = 870U;
constexpr std::uint32_t SaleBuyerIndexOffset = 0U;
constexpr std::uint32_t SaleSoldItemsOffset = 1U;
constexpr std::uint32_t SaleAmountOffset = 2U;
constexpr std::uint32_t SaleAmountWithMembershipModifiersOffset = 3U;
constexpr std::uint32_t TotalSaleAmountWithMembershipModifiersOffset = 4U;
constexpr std::uint32_t TotalSaleAmountOffset = 5U;
constexpr std::uint32_t SaleUnknownPartialAmountOffset = 6U;
constexpr std::uint32_t SaleBuyerIndex2Offset = 7U;

// GPBD_FM_3 entry offsets verified against the b1158.13 615-slot layout.
constexpr std::uint32_t NightclubMissionIndexOffset = 491U;
constexpr std::uint32_t NightclubDefendMissionIndexOffset = 492U;

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

struct NightclubActionResult
{
    GTA_Nightclub_Action_Status status = GTA_Nightclub_Action_Status::Failed;
    std::string detail;
};

struct ResupplyActionResult
{
    GTA_Resupply_Action_Status status = GTA_Resupply_Action_Status::Failed;
    std::string detail;
};

template <typename T>
bool ReadGlobal(Script_Global_Manager& globals, std::uint32_t index, T& value)
{
    auto result = globals.Get(index).Read<T>();
    if (!result)
        return false;

    value = result.Value();
    return true;
}

template <typename T>
bool WriteGlobalVerified(
    Script_Global_Manager& globals,
    std::uint32_t index,
    const T& value)
{
    static_assert(std::is_trivially_copyable_v<T>);

    auto pointer = globals.Get(index).Resolve();
    if (!pointer)
        return false;

    std::memcpy(
        reinterpret_cast<void*>(pointer.Value().Address()),
        &value,
        sizeof(value));

    auto readback = globals.Get(index).Read<T>();
    if (!readback)
        return false;

    const auto actual = readback.Value();
    return std::memcmp(&actual, &value, sizeof(T)) == 0;
}

std::uint32_t PlayerGpbdFmEntry(int player) noexcept
{
    return GpbdFmBase +
        1U +
        static_cast<std::uint32_t>(player) * GpbdFmPlayerStride;
}

std::uint32_t PlayerPropertyData(int player) noexcept
{
    return PlayerGpbdFmEntry(player) + PropertyDataOffset;
}

std::uint32_t PlayerGpbdFm3Entry(int player) noexcept
{
    return GpbdFm3Base +
        1U +
        static_cast<std::uint32_t>(player) * GpbdFm3PlayerStride;
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
    const auto entry = PlayerGpbdFmEntry(*player);
    const auto property = PlayerPropertyData(*player);
    const auto missionEntry = PlayerGpbdFm3Entry(*player);
    const auto sale = entry + NightclubSaleOffset;

    bool liveOk = true;
    liveOk &= ReadGlobal(
        *globals,
        property + BusinessHubOffset,
        snapshot.businessHubIndex);
    liveOk &= ReadGlobal(*globals, property + NightclubDataOffset, snapshot.nightclubIndex);
    liveOk &= ReadGlobal(*globals, property + NightclubPopularityOffset, snapshot.popularity);
    liveOk &= ReadGlobal(*globals, property + NightclubSafeOffset, snapshot.safeCash);
    liveOk &= ReadGlobal(*globals, property + NightclubEntryCostOffset, snapshot.entryCost);

    liveOk &= ReadGlobal(
        *globals,
        sale + SaleBuyerIndexOffset,
        snapshot.sale.buyerIndex);
    liveOk &= ReadGlobal(
        *globals,
        sale + SaleSoldItemsOffset,
        snapshot.sale.values.soldItems);
    liveOk &= ReadGlobal(
        *globals,
        sale + SaleAmountOffset,
        snapshot.sale.values.saleAmount);
    liveOk &= ReadGlobal(
        *globals,
        sale + SaleAmountWithMembershipModifiersOffset,
        snapshot.sale.values.saleAmountWithMembershipModifiers);
    liveOk &= ReadGlobal(
        *globals,
        sale + TotalSaleAmountWithMembershipModifiersOffset,
        snapshot.sale.values.totalSaleAmountWithMembershipModifiers);
    liveOk &= ReadGlobal(
        *globals,
        sale + TotalSaleAmountOffset,
        snapshot.sale.values.totalSaleAmount);
    liveOk &= ReadGlobal(
        *globals,
        sale + SaleUnknownPartialAmountOffset,
        snapshot.sale.unknownPartialAmount);
    liveOk &= ReadGlobal(
        *globals,
        sale + SaleBuyerIndex2Offset,
        snapshot.sale.buyerIndex2);

    liveOk &= ReadGlobal(
        *globals,
        missionEntry + NightclubMissionIndexOffset,
        snapshot.missionIndex);
    liveOk &= ReadGlobal(
        *globals,
        missionEntry + NightclubDefendMissionIndexOffset,
        snapshot.defendMissionIndex);

    if (!liveOk) {
        snapshot.detail = "Nightclub live state could not be read.";
        return snapshot;
    }

    snapshot.owned = snapshot.nightclubIndex > 0 &&
        snapshot.nightclubIndex == snapshot.businessHubIndex;

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

        std::uint64_t units = 0;

        goodOk &= ReadGlobal(
            *globals,
            property + ProductStocksOffset + definition.liveStockIndex,
            units);
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

        good.units = static_cast<int>(std::min(
            units,
            static_cast<std::uint64_t>((std::numeric_limits<int>::max)())));
        good.available = goodOk;
    }

    snapshot.runtimeReady = true;
    snapshot.detail = snapshot.owned
        ? "Nightclub business data ready."
        : "Nightclub runtime ready; no owned Nightclub detected.";
    return snapshot;
}

NightclubActionResult ExecuteNightclubAction(
    GTA_Native_Manager& natives,
    const GTA_Nightclub_Action_Command& command)
{
    auto* globals = g_runtime.globals;
    if (!globals || !globals->Ready()) {
        return {
            GTA_Nightclub_Action_Status::RuntimeUnavailable,
            "Script globals are not configured."};
    }

    if (g_runtime.buildFingerprint != SupportedFingerprint) {
        return {
            GTA_Nightclub_Action_Status::UnsupportedBuild,
            "Nightclub writes are not registered for this GTA build."};
    }

    const auto player = natives.Invoke<int>(GTA_Native_Id::PlayerId);
    if (!player || *player < 0 || *player >= 32) {
        return {
            GTA_Nightclub_Action_Status::RuntimeUnavailable,
            "Local player index is unavailable."};
    }

    const auto entry = PlayerGpbdFmEntry(*player);
    const auto property = PlayerPropertyData(*player);
    const auto missionEntry = PlayerGpbdFm3Entry(*player);
    const auto sale = entry + NightclubSaleOffset;

    int nightclubIndex = 0;
    int businessHubIndex = 0;
    if (!ReadGlobal(
            *globals,
            property + NightclubDataOffset,
            nightclubIndex) ||
        !ReadGlobal(
            *globals,
            property + BusinessHubOffset,
            businessHubIndex)) {
        return {
            GTA_Nightclub_Action_Status::RuntimeUnavailable,
            "Nightclub ownership globals could not be read."};
    }

    if (nightclubIndex <= 0 || nightclubIndex != businessHubIndex) {
        return {
            GTA_Nightclub_Action_Status::NoNightclub,
            "No owned Nightclub was found for the local player."};
    }

    const auto writeFailed = [] {
        return NightclubActionResult{
            GTA_Nightclub_Action_Status::Failed,
            "A Nightclub global write failed readback verification."};
    };

    switch (command.kind) {
    case GTA_Nightclub_Action_Kind::SetCoreValues: {
        if (!std::isfinite(command.popularity) ||
            command.safeCash < 0 ||
            command.entryCost < 0) {
            return {
                GTA_Nightclub_Action_Status::InvalidValue,
                "Popularity must be finite and cash values cannot be negative."};
        }

        const float popularity = std::clamp(command.popularity, 0.0F, 1.0F);
        if (!WriteGlobalVerified(
                *globals,
                property + NightclubPopularityOffset,
                popularity) ||
            !WriteGlobalVerified(
                *globals,
                property + NightclubSafeOffset,
                command.safeCash) ||
            !WriteGlobalVerified(
                *globals,
                property + NightclubEntryCostOffset,
                command.entryCost)) {
            return writeFailed();
        }

        return {
            GTA_Nightclub_Action_Status::Succeeded,
            "Popularity, safe cash, and entry cost were applied and verified."};
    }

    case GTA_Nightclub_Action_Kind::SetPopularity: {
        if (!std::isfinite(command.popularity)) {
            return {
                GTA_Nightclub_Action_Status::InvalidValue,
                "Popularity must be a finite number."};
        }

        const float popularity = std::clamp(command.popularity, 0.0F, 1.0F);
        if (!WriteGlobalVerified(
                *globals,
                property + NightclubPopularityOffset,
                popularity)) {
            return writeFailed();
        }

        return {
            GTA_Nightclub_Action_Status::Succeeded,
            "Nightclub popularity was applied and verified."};
    }

    case GTA_Nightclub_Action_Kind::SetSafeCash:
        if (command.safeCash < 0) {
            return {
                GTA_Nightclub_Action_Status::InvalidValue,
                "Safe cash cannot be negative."};
        }
        if (!WriteGlobalVerified(
                *globals,
                property + NightclubSafeOffset,
                command.safeCash)) {
            return writeFailed();
        }
        return {
            GTA_Nightclub_Action_Status::Succeeded,
            "Nightclub safe cash was applied and verified."};

    case GTA_Nightclub_Action_Kind::SetEntryCost:
        if (command.entryCost < 0) {
            return {
                GTA_Nightclub_Action_Status::InvalidValue,
                "Entry cost cannot be negative."};
        }
        if (!WriteGlobalVerified(
                *globals,
                property + NightclubEntryCostOffset,
                command.entryCost)) {
            return writeFailed();
        }
        return {
            GTA_Nightclub_Action_Status::Succeeded,
            "Nightclub entry cost was applied and verified."};

    case GTA_Nightclub_Action_Kind::SetProductStocks:
    case GTA_Nightclub_Action_Kind::FillProductStocks:
    case GTA_Nightclub_Action_Kind::ClearProductStocks: {
        std::array<int, GTA_Nightclub_Good_Count> targetStocks{};

        for (const auto& definition : GoodDefinitions) {
            const auto index = static_cast<std::size_t>(definition.good);
            int maxUnits = 0;
            if (!ReadGlobal(
                    *globals,
                    TunablesBase + definition.maxUnitsOffset,
                    maxUnits) ||
                maxUnits < 0) {
                return {
                    GTA_Nightclub_Action_Status::RuntimeUnavailable,
                    "Nightclub warehouse capacity tunables could not be read."};
            }

            if (command.kind == GTA_Nightclub_Action_Kind::FillProductStocks)
                targetStocks[index] = maxUnits;
            else if (command.kind == GTA_Nightclub_Action_Kind::ClearProductStocks)
                targetStocks[index] = 0;
            else
                targetStocks[index] = std::clamp(command.productStocks[index], 0, maxUnits);
        }

        for (const auto& definition : GoodDefinitions) {
            const auto index = static_cast<std::size_t>(definition.good);
            const auto units = static_cast<std::uint64_t>(targetStocks[index]);
            if (!WriteGlobalVerified(
                    *globals,
                    property + ProductStocksOffset + definition.liveStockIndex,
                    units)) {
                return writeFailed();
            }
        }

        const char* detail = command.kind == GTA_Nightclub_Action_Kind::FillProductStocks
            ? "Nightclub warehouse stock was filled and verified."
            : command.kind == GTA_Nightclub_Action_Kind::ClearProductStocks
                ? "Nightclub warehouse stock was cleared and verified."
                : "Nightclub warehouse stock was applied and verified.";
        return {GTA_Nightclub_Action_Status::Succeeded, detail};
    }

    case GTA_Nightclub_Action_Kind::SetSaleValues: {
        const auto& values = command.sale;
        if (values.soldItems < 0 ||
            values.saleAmount < 0 ||
            values.saleAmountWithMembershipModifiers < 0 ||
            values.totalSaleAmountWithMembershipModifiers < 0 ||
            values.totalSaleAmount < 0) {
            return {
                GTA_Nightclub_Action_Status::InvalidValue,
                "Nightclub sale values cannot be negative."};
        }

        if (!WriteGlobalVerified(
                *globals,
                sale + SaleSoldItemsOffset,
                values.soldItems) ||
            !WriteGlobalVerified(
                *globals,
                sale + SaleAmountOffset,
                values.saleAmount) ||
            !WriteGlobalVerified(
                *globals,
                sale + SaleAmountWithMembershipModifiersOffset,
                values.saleAmountWithMembershipModifiers) ||
            !WriteGlobalVerified(
                *globals,
                sale + TotalSaleAmountWithMembershipModifiersOffset,
                values.totalSaleAmountWithMembershipModifiers) ||
            !WriteGlobalVerified(
                *globals,
                sale + TotalSaleAmountOffset,
                values.totalSaleAmount)) {
            return writeFailed();
        }

        return {
            GTA_Nightclub_Action_Status::Succeeded,
            "Nightclub live sale values were applied and verified."};
    }

    case GTA_Nightclub_Action_Kind::SetMissionState:
    case GTA_Nightclub_Action_Kind::ResetMissionState: {
        const int missionIndex = command.kind == GTA_Nightclub_Action_Kind::ResetMissionState
            ? -1
            : std::max(command.missionIndex, -1);
        const int defendMissionIndex = command.kind == GTA_Nightclub_Action_Kind::ResetMissionState
            ? -1
            : std::max(command.defendMissionIndex, -1);

        if (!WriteGlobalVerified(
                *globals,
                missionEntry + NightclubMissionIndexOffset,
                missionIndex) ||
            !WriteGlobalVerified(
                *globals,
                missionEntry + NightclubDefendMissionIndexOffset,
                defendMissionIndex)) {
            return writeFailed();
        }

        return {
            GTA_Nightclub_Action_Status::Succeeded,
            command.kind == GTA_Nightclub_Action_Kind::ResetMissionState
                ? "Nightclub mission indexes were reset and verified."
                : "Nightclub mission indexes were applied and verified."};
    }

    default:
        return {
            GTA_Nightclub_Action_Status::InvalidValue,
            "The requested Nightclub action is invalid."};
    }
}

ResupplyActionResult ExecuteResupplyAction(
    const GTA_Resupply_Action_Command& command)
{
    auto* globals = g_runtime.globals;
    if (!globals || !globals->Ready()) {
        return {
            GTA_Resupply_Action_Status::RuntimeUnavailable,
            "Script globals are not configured."};
    }

    if (g_runtime.buildFingerprint != SupportedFingerprint) {
        return {
            GTA_Resupply_Action_Status::UnsupportedBuild,
            "Instant-resupply globals are not registered for this GTA build."};
    }

    const auto writeTarget = [globals](GTA_Resupply_Target target) {
        const auto index = static_cast<std::uint32_t>(target);
        return WriteGlobalVerified(
            *globals,
            FreemodeBusinessBase + 1U + index,
            InstantResupplyTrigger);
    };

    switch (command.kind) {
    case GTA_Resupply_Action_Kind::ResupplyTarget: {
        const auto index = static_cast<std::size_t>(command.target);
        if (index >= GTA_Resupply_Target_Count) {
            return {
                GTA_Resupply_Action_Status::InvalidTarget,
                "The requested resupply target is invalid."};
        }

        if (!writeTarget(command.target)) {
            return {
                GTA_Resupply_Action_Status::Failed,
                "The instant-resupply trigger failed readback verification."};
        }

        return {
            GTA_Resupply_Action_Status::Succeeded,
            std::string{"Instant resupply triggered for "} +
                GTA_Resupply_Target_Name(command.target) + "."};
    }

    case GTA_Resupply_Action_Kind::ResupplyAll:
        for (std::size_t index = 0; index < GTA_Resupply_Target_Count; ++index) {
            if (!writeTarget(static_cast<GTA_Resupply_Target>(index))) {
                return {
                    GTA_Resupply_Action_Status::Failed,
                    "One or more instant-resupply triggers failed readback verification."};
            }
        }

        return {
            GTA_Resupply_Action_Status::Succeeded,
            "Instant resupply triggered for all five MC slots, the Bunker, and the Acid Lab."};

    default:
        return {
            GTA_Resupply_Action_Status::InvalidTarget,
            "The requested resupply action is invalid."};
    }
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
    auto& state = GTA_Business_State::Instance();
    GTA_Nightclub_Action_Command nightclubCommand{};
    const bool nightclubActionProcessed =
        state.ConsumeNightclubAction(nightclubCommand);

    if (nightclubActionProcessed) {
        auto result = ExecuteNightclubAction(natives, nightclubCommand);
        state.CompleteNightclubAction(
            nightclubCommand,
            result.status,
            std::move(result.detail));
    }

    GTA_Resupply_Action_Command resupplyCommand{};
    const bool resupplyActionProcessed =
        state.ConsumeResupplyAction(resupplyCommand);

    if (resupplyActionProcessed) {
        auto result = ExecuteResupplyAction(resupplyCommand);
        state.CompleteResupplyAction(
            resupplyCommand,
            result.status,
            std::move(result.detail));
    }

    const bool actionProcessed =
        nightclubActionProcessed || resupplyActionProcessed;

    if (!g_runtime.globals)
        return;

    const auto now = std::chrono::steady_clock::now();
    if (!actionProcessed &&
        g_runtime.lastRefresh != std::chrono::steady_clock::time_point{} &&
        now - g_runtime.lastRefresh < RefreshInterval) {
        return;
    }

    g_runtime.lastRefresh = now;
    state.PublishNightclub(BuildNightclubSnapshot(natives));
}
}
