#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <utility>

namespace Devilz::Integrations::GTA5_Enhanced
{
enum class GTA_Nightclub_Good : std::uint8_t
{
    Cargo,
    SportingGoods,
    SouthAmericanImports,
    PharmaceuticalResearch,
    OrganicProduce,
    PrintingAndCopying,
    CashCreation,
    Count
};

inline constexpr std::size_t GTA_Nightclub_Good_Count =
    static_cast<std::size_t>(GTA_Nightclub_Good::Count);

inline constexpr int GTA_Nightclub_Safe_Capacity = 250'000;

struct GTA_Nightclub_Good_Snapshot
{
    bool available = false;
    int units = 0;
    int maxUnits = 0;
    int unitValue = 0;
    int specialOrderUnitValue = 0;
    int productionTimeMs = 0;
};

struct GTA_Nightclub_Sale_Values
{
    int soldItems = 0;
    int saleAmount = 0;
    int saleAmountWithMembershipModifiers = 0;
    int totalSaleAmountWithMembershipModifiers = 0;
    int totalSaleAmount = 0;
};

struct GTA_Nightclub_Sale_Snapshot
{
    int buyerIndex = 0;
    int buyerIndex2 = 0;
    int unknownPartialAmount = 0;
    GTA_Nightclub_Sale_Values values{};
};

struct GTA_Nightclub_Snapshot
{
    bool runtimeReady = false;
    bool owned = false;
    int playerIndex = -1;
    int nightclubIndex = 0;
    int businessHubIndex = 0;
    float popularity = 0.0F;
    int safeCash = 0;
    int entryCost = 0;
    float equipmentUpgradeMultiplier = 1.0F;
    int managementMissionCooldownMs = 0;
    int sellMissionCooldownMs = 0;
    int specialOrderSellCooldownMs = 0;
    std::array<GTA_Nightclub_Good_Snapshot, GTA_Nightclub_Good_Count> goods{};
    std::array<int, 20> popularityIncome{};
    GTA_Nightclub_Sale_Snapshot sale{};
    int missionIndex = -1;
    int defendMissionIndex = -1;
    std::string detail = "Nightclub runtime is unavailable.";
};

enum class GTA_Nightclub_Action_Kind : std::uint8_t
{
    None,
    SetCoreValues,
    SetPopularity,
    SetSafeCash,
    SetEntryCost,
    SetProductStocks,
    FillProductStocks,
    ClearProductStocks,
    SetSaleValues,
    SetMissionState,
    ResetMissionState
};

enum class GTA_Nightclub_Action_Status : std::uint8_t
{
    Idle,
    Queued,
    Succeeded,
    RuntimeUnavailable,
    UnsupportedBuild,
    NoNightclub,
    InvalidValue,
    Failed
};

struct GTA_Nightclub_Action_Snapshot
{
    std::uint64_t revision = 0;
    std::uint64_t requestId = 0;
    GTA_Nightclub_Action_Kind kind = GTA_Nightclub_Action_Kind::None;
    GTA_Nightclub_Action_Status status = GTA_Nightclub_Action_Status::Idle;
    std::string detail;
};

struct GTA_Nightclub_Action_Command
{
    std::uint64_t id = 0;
    GTA_Nightclub_Action_Kind kind = GTA_Nightclub_Action_Kind::None;
    float popularity = 0.0F;
    int safeCash = 0;
    int entryCost = 0;
    std::array<int, GTA_Nightclub_Good_Count> productStocks{};
    GTA_Nightclub_Sale_Values sale{};
    int missionIndex = -1;
    int defendMissionIndex = -1;
};

enum class GTA_Resupply_Target : std::uint8_t
{
    McBusinessSlot0 = 0,
    McBusinessSlot1 = 1,
    McBusinessSlot2 = 2,
    McBusinessSlot3 = 3,
    McBusinessSlot4 = 4,
    Bunker = 5,
    AcidLab = 6,
    Count = 7
};

inline constexpr std::size_t GTA_Resupply_Target_Count =
    static_cast<std::size_t>(GTA_Resupply_Target::Count);

enum class GTA_Resupply_Action_Kind : std::uint8_t
{
    None,
    ResupplyTarget,
    ResupplyAll
};

enum class GTA_Resupply_Action_Status : std::uint8_t
{
    Idle,
    Queued,
    Succeeded,
    RuntimeUnavailable,
    UnsupportedBuild,
    InvalidTarget,
    Failed
};

struct GTA_Resupply_Action_Snapshot
{
    std::uint64_t revision = 0;
    std::uint64_t requestId = 0;
    GTA_Resupply_Action_Kind kind = GTA_Resupply_Action_Kind::None;
    GTA_Resupply_Target target = GTA_Resupply_Target::McBusinessSlot0;
    GTA_Resupply_Action_Status status = GTA_Resupply_Action_Status::Idle;
    std::string detail;
};

struct GTA_Resupply_Action_Command
{
    std::uint64_t id = 0;
    GTA_Resupply_Action_Kind kind = GTA_Resupply_Action_Kind::None;
    GTA_Resupply_Target target = GTA_Resupply_Target::McBusinessSlot0;
};

inline constexpr std::size_t GTA_Special_Cargo_Warehouse_Count = 5;
inline constexpr int GTA_Special_Cargo_Min_Sourcing_Amount = 1;
inline constexpr int GTA_Special_Cargo_Max_Sourcing_Amount = 111;
inline constexpr int GTA_Special_Cargo_Min_Type = -1;
inline constexpr int GTA_Special_Cargo_Max_Type = 10;
inline constexpr int GTA_Special_Cargo_Min_Item = 0;
inline constexpr int GTA_Special_Cargo_Max_Item = 5;
inline constexpr int GTA_Special_Cargo_Max_Held = 111;

struct GTA_Special_Cargo_Warehouse_Snapshot
{
    bool available = false;
    bool owned = false;
    int propertyId = 0;
    int cargoHeld = 0;
};

struct GTA_Special_Cargo_Snapshot
{
    bool runtimeReady = false;
    int playerIndex = -1;
    int sourcingAmount = GTA_Special_Cargo_Min_Sourcing_Amount;
    int cargoType = GTA_Special_Cargo_Min_Type;
    int specialItem = GTA_Special_Cargo_Min_Item;
    bool specialItemAvailable = false;
    std::array<GTA_Special_Cargo_Warehouse_Snapshot, GTA_Special_Cargo_Warehouse_Count> warehouses{};
    std::string detail = "Special Cargo runtime is unavailable.";
};

enum class GTA_Special_Cargo_Action_Kind : std::uint8_t
{
    None,
    ApplySourcingSettings,
    SetWarehouseCargo
};

enum class GTA_Special_Cargo_Action_Status : std::uint8_t
{
    Idle,
    Queued,
    Succeeded,
    RuntimeUnavailable,
    UnsupportedBuild,
    NoWarehouse,
    InvalidValue,
    Failed
};

struct GTA_Special_Cargo_Action_Snapshot
{
    std::uint64_t revision = 0;
    std::uint64_t requestId = 0;
    GTA_Special_Cargo_Action_Kind kind = GTA_Special_Cargo_Action_Kind::None;
    std::size_t warehouseSlot = 0;
    GTA_Special_Cargo_Action_Status status = GTA_Special_Cargo_Action_Status::Idle;
    std::string detail;
};

struct GTA_Special_Cargo_Action_Command
{
    std::uint64_t id = 0;
    GTA_Special_Cargo_Action_Kind kind = GTA_Special_Cargo_Action_Kind::None;
    int sourcingAmount = GTA_Special_Cargo_Min_Sourcing_Amount;
    int cargoType = GTA_Special_Cargo_Min_Type;
    int specialItem = GTA_Special_Cargo_Min_Item;
    bool specialItemAvailable = false;
    std::size_t warehouseSlot = 0;
    int cargoHeld = 0;
};

inline constexpr std::uint32_t GTA_Vehicle_Cargo_Steal_Cooldown_Offset = 19170U;
inline constexpr std::array<std::uint32_t, 4> GTA_Vehicle_Cargo_Sell_Cooldown_Offsets{{
    19525U,
    19526U,
    19527U,
    19528U,
}};
inline constexpr std::uint32_t GTA_Vehicle_Cargo_Top_Range_Value_Offset = 19263U;
inline constexpr std::uint32_t GTA_Vehicle_Cargo_Mid_Range_Value_Offset = 19264U;
inline constexpr std::uint32_t GTA_Vehicle_Cargo_Standard_Range_Value_Offset = 19265U;

inline constexpr int GTA_Vehicle_Cargo_Supplied_Steal_Cooldown_Ms = 180'000;
inline constexpr std::array<int, 4> GTA_Vehicle_Cargo_Supplied_Sell_Cooldowns_Ms{{
    1'200'000,
    1'680'000,
    2'340'000,
    2'880'000,
}};
inline constexpr int GTA_Vehicle_Cargo_Supplied_Top_Range_Value = 40'000;
inline constexpr int GTA_Vehicle_Cargo_Supplied_Mid_Range_Value = 25'000;
inline constexpr int GTA_Vehicle_Cargo_Supplied_Standard_Range_Value = 15'000;

struct GTA_Vehicle_Cargo_Values
{
    int stealMissionCooldownMs = GTA_Vehicle_Cargo_Supplied_Steal_Cooldown_Ms;
    std::array<int, 4> sellMissionCooldownMs = GTA_Vehicle_Cargo_Supplied_Sell_Cooldowns_Ms;
    int topRangeValue = GTA_Vehicle_Cargo_Supplied_Top_Range_Value;
    int midRangeValue = GTA_Vehicle_Cargo_Supplied_Mid_Range_Value;
    int standardRangeValue = GTA_Vehicle_Cargo_Supplied_Standard_Range_Value;
};

inline constexpr GTA_Vehicle_Cargo_Values GTA_Vehicle_Cargo_Supplied_173_Values{};

struct GTA_Vehicle_Cargo_Snapshot
{
    bool runtimeReady = false;
    GTA_Vehicle_Cargo_Values values{};
    std::string detail = "Vehicle Cargo tunables are unavailable.";
};

enum class GTA_Vehicle_Cargo_Action_Kind : std::uint8_t
{
    None,
    ApplyCooldowns,
    ApplySaleValues,
    ApplyAll
};

enum class GTA_Vehicle_Cargo_Action_Status : std::uint8_t
{
    Idle,
    Queued,
    Succeeded,
    RuntimeUnavailable,
    UnsupportedBuild,
    InvalidValue,
    Failed
};

struct GTA_Vehicle_Cargo_Action_Snapshot
{
    std::uint64_t revision = 0;
    std::uint64_t requestId = 0;
    GTA_Vehicle_Cargo_Action_Kind kind = GTA_Vehicle_Cargo_Action_Kind::None;
    GTA_Vehicle_Cargo_Action_Status status = GTA_Vehicle_Cargo_Action_Status::Idle;
    std::string detail;
};

struct GTA_Vehicle_Cargo_Action_Command
{
    std::uint64_t id = 0;
    GTA_Vehicle_Cargo_Action_Kind kind = GTA_Vehicle_Cargo_Action_Kind::None;
    GTA_Vehicle_Cargo_Values values{};
};

class GTA_Native_Manager;

class GTA_Business_State final
{
public:
    static GTA_Business_State& Instance() noexcept
    {
        static GTA_Business_State state;
        return state;
    }

    GTA_Business_State(const GTA_Business_State&) = delete;
    GTA_Business_State& operator=(const GTA_Business_State&) = delete;

    [[nodiscard]] GTA_Nightclub_Snapshot Nightclub() const
    {
        std::scoped_lock lock(m_mutex);
        return m_nightclub;
    }

    [[nodiscard]] GTA_Nightclub_Action_Snapshot NightclubAction() const
    {
        std::scoped_lock lock(m_mutex);
        return m_nightclubAction;
    }

    [[nodiscard]] GTA_Resupply_Action_Snapshot ResupplyAction() const
    {
        std::scoped_lock lock(m_mutex);
        return m_resupplyAction;
    }

    [[nodiscard]] GTA_Special_Cargo_Snapshot SpecialCargo() const
    {
        std::scoped_lock lock(m_mutex);
        return m_specialCargo;
    }

    [[nodiscard]] GTA_Special_Cargo_Action_Snapshot SpecialCargoAction() const
    {
        std::scoped_lock lock(m_mutex);
        return m_specialCargoAction;
    }

    [[nodiscard]] GTA_Vehicle_Cargo_Snapshot VehicleCargo() const
    {
        std::scoped_lock lock(m_mutex);
        return m_vehicleCargo;
    }

    [[nodiscard]] GTA_Vehicle_Cargo_Action_Snapshot VehicleCargoAction() const
    {
        std::scoped_lock lock(m_mutex);
        return m_vehicleCargoAction;
    }

    [[nodiscard]] bool RequestNightclubCoreValues(
        float popularity,
        int safeCash,
        int entryCost)
    {
        GTA_Nightclub_Action_Command command{};
        command.kind = GTA_Nightclub_Action_Kind::SetCoreValues;
        command.popularity = popularity;
        command.safeCash = safeCash;
        command.entryCost = entryCost;
        return QueueNightclubAction(std::move(command));
    }

    [[nodiscard]] bool RequestNightclubPopularity(float popularity)
    {
        GTA_Nightclub_Action_Command command{};
        command.kind = GTA_Nightclub_Action_Kind::SetPopularity;
        command.popularity = popularity;
        return QueueNightclubAction(std::move(command));
    }

    [[nodiscard]] bool RequestNightclubSafeCash(int safeCash)
    {
        GTA_Nightclub_Action_Command command{};
        command.kind = GTA_Nightclub_Action_Kind::SetSafeCash;
        command.safeCash = safeCash;
        return QueueNightclubAction(std::move(command));
    }

    [[nodiscard]] bool RequestNightclubEntryCost(int entryCost)
    {
        GTA_Nightclub_Action_Command command{};
        command.kind = GTA_Nightclub_Action_Kind::SetEntryCost;
        command.entryCost = entryCost;
        return QueueNightclubAction(std::move(command));
    }

    [[nodiscard]] bool RequestNightclubProductStocks(
        std::array<int, GTA_Nightclub_Good_Count> productStocks)
    {
        GTA_Nightclub_Action_Command command{};
        command.kind = GTA_Nightclub_Action_Kind::SetProductStocks;
        command.productStocks = std::move(productStocks);
        return QueueNightclubAction(std::move(command));
    }

    [[nodiscard]] bool RequestFillNightclubProductStocks()
    {
        GTA_Nightclub_Action_Command command{};
        command.kind = GTA_Nightclub_Action_Kind::FillProductStocks;
        return QueueNightclubAction(std::move(command));
    }

    [[nodiscard]] bool RequestClearNightclubProductStocks()
    {
        GTA_Nightclub_Action_Command command{};
        command.kind = GTA_Nightclub_Action_Kind::ClearProductStocks;
        return QueueNightclubAction(std::move(command));
    }

    [[nodiscard]] bool RequestNightclubSaleValues(GTA_Nightclub_Sale_Values sale)
    {
        GTA_Nightclub_Action_Command command{};
        command.kind = GTA_Nightclub_Action_Kind::SetSaleValues;
        command.sale = std::move(sale);
        return QueueNightclubAction(std::move(command));
    }

    [[nodiscard]] bool RequestNightclubMissionState(
        int missionIndex,
        int defendMissionIndex)
    {
        GTA_Nightclub_Action_Command command{};
        command.kind = GTA_Nightclub_Action_Kind::SetMissionState;
        command.missionIndex = missionIndex;
        command.defendMissionIndex = defendMissionIndex;
        return QueueNightclubAction(std::move(command));
    }

    [[nodiscard]] bool RequestResetNightclubMissionState()
    {
        GTA_Nightclub_Action_Command command{};
        command.kind = GTA_Nightclub_Action_Kind::ResetMissionState;
        return QueueNightclubAction(std::move(command));
    }

    [[nodiscard]] bool RequestInstantResupply(GTA_Resupply_Target target)
    {
        GTA_Resupply_Action_Command command{};
        command.kind = GTA_Resupply_Action_Kind::ResupplyTarget;
        command.target = target;
        return QueueResupplyAction(std::move(command));
    }

    [[nodiscard]] bool RequestInstantResupplyAll()
    {
        GTA_Resupply_Action_Command command{};
        command.kind = GTA_Resupply_Action_Kind::ResupplyAll;
        return QueueResupplyAction(std::move(command));
    }

    [[nodiscard]] bool RequestSpecialCargoSourcingSettings(
        int sourcingAmount,
        int cargoType,
        int specialItem,
        bool specialItemAvailable)
    {
        GTA_Special_Cargo_Action_Command command{};
        command.kind = GTA_Special_Cargo_Action_Kind::ApplySourcingSettings;
        command.sourcingAmount = sourcingAmount;
        command.cargoType = cargoType;
        command.specialItem = specialItem;
        command.specialItemAvailable = specialItemAvailable;
        return QueueSpecialCargoAction(std::move(command));
    }

    [[nodiscard]] bool RequestSpecialCargoWarehouseCargo(
        std::size_t warehouseSlot,
        int cargoHeld)
    {
        GTA_Special_Cargo_Action_Command command{};
        command.kind = GTA_Special_Cargo_Action_Kind::SetWarehouseCargo;
        command.warehouseSlot = warehouseSlot;
        command.cargoHeld = cargoHeld;
        return QueueSpecialCargoAction(std::move(command));
    }

    [[nodiscard]] bool RequestVehicleCargoCooldowns(GTA_Vehicle_Cargo_Values values)
    {
        GTA_Vehicle_Cargo_Action_Command command{};
        command.kind = GTA_Vehicle_Cargo_Action_Kind::ApplyCooldowns;
        command.values = std::move(values);
        return QueueVehicleCargoAction(std::move(command));
    }

    [[nodiscard]] bool RequestVehicleCargoSaleValues(GTA_Vehicle_Cargo_Values values)
    {
        GTA_Vehicle_Cargo_Action_Command command{};
        command.kind = GTA_Vehicle_Cargo_Action_Kind::ApplySaleValues;
        command.values = std::move(values);
        return QueueVehicleCargoAction(std::move(command));
    }

    [[nodiscard]] bool RequestVehicleCargoAll(GTA_Vehicle_Cargo_Values values)
    {
        GTA_Vehicle_Cargo_Action_Command command{};
        command.kind = GTA_Vehicle_Cargo_Action_Kind::ApplyAll;
        command.values = std::move(values);
        return QueueVehicleCargoAction(std::move(command));
    }

    void PublishNightclub(GTA_Nightclub_Snapshot snapshot)
    {
        std::scoped_lock lock(m_mutex);
        m_nightclub = std::move(snapshot);
    }

    void PublishSpecialCargo(GTA_Special_Cargo_Snapshot snapshot)
    {
        std::scoped_lock lock(m_mutex);
        m_specialCargo = std::move(snapshot);
    }

    void PublishVehicleCargo(GTA_Vehicle_Cargo_Snapshot snapshot)
    {
        std::scoped_lock lock(m_mutex);
        m_vehicleCargo = std::move(snapshot);
    }

    void Reset()
    {
        std::scoped_lock lock(m_mutex);
        m_nightclub = {};
        m_pendingNightclubAction.reset();
        m_nightclubAction = {};
        m_nextNightclubActionId = 1;
        m_pendingResupplyAction.reset();
        m_resupplyAction = {};
        m_nextResupplyActionId = 1;
        m_specialCargo = {};
        m_pendingSpecialCargoAction.reset();
        m_specialCargoAction = {};
        m_nextSpecialCargoActionId = 1;
        m_vehicleCargo = {};
        m_pendingVehicleCargoAction.reset();
        m_vehicleCargoAction = {};
        m_nextVehicleCargoActionId = 1;
    }

private:
    GTA_Business_State() = default;

    [[nodiscard]] bool QueueNightclubAction(GTA_Nightclub_Action_Command command)
    {
        std::scoped_lock lock(m_mutex);
        if (m_pendingNightclubAction ||
            m_nightclubAction.status == GTA_Nightclub_Action_Status::Queued) {
            return false;
        }

        command.id = m_nextNightclubActionId++;
        m_pendingNightclubAction = std::move(command);

        ++m_nightclubAction.revision;
        m_nightclubAction.requestId = m_pendingNightclubAction->id;
        m_nightclubAction.kind = m_pendingNightclubAction->kind;
        m_nightclubAction.status = GTA_Nightclub_Action_Status::Queued;
        m_nightclubAction.detail = "Queued for the GTA game thread.";
        return true;
    }

    [[nodiscard]] bool ConsumeNightclubAction(GTA_Nightclub_Action_Command& command)
    {
        std::scoped_lock lock(m_mutex);
        if (!m_pendingNightclubAction)
            return false;

        command = std::move(*m_pendingNightclubAction);
        m_pendingNightclubAction.reset();
        return true;
    }

    void CompleteNightclubAction(
        const GTA_Nightclub_Action_Command& command,
        GTA_Nightclub_Action_Status status,
        std::string detail)
    {
        std::scoped_lock lock(m_mutex);
        if (m_nightclubAction.requestId != command.id)
            return;

        ++m_nightclubAction.revision;
        m_nightclubAction.status = status;
        m_nightclubAction.detail = std::move(detail);
    }

    [[nodiscard]] bool QueueResupplyAction(GTA_Resupply_Action_Command command)
    {
        std::scoped_lock lock(m_mutex);
        if (m_pendingResupplyAction ||
            m_resupplyAction.status == GTA_Resupply_Action_Status::Queued) {
            return false;
        }

        command.id = m_nextResupplyActionId++;
        m_pendingResupplyAction = std::move(command);

        ++m_resupplyAction.revision;
        m_resupplyAction.requestId = m_pendingResupplyAction->id;
        m_resupplyAction.kind = m_pendingResupplyAction->kind;
        m_resupplyAction.target = m_pendingResupplyAction->target;
        m_resupplyAction.status = GTA_Resupply_Action_Status::Queued;
        m_resupplyAction.detail = "Queued for the GTA game thread.";
        return true;
    }

    [[nodiscard]] bool ConsumeResupplyAction(GTA_Resupply_Action_Command& command)
    {
        std::scoped_lock lock(m_mutex);
        if (!m_pendingResupplyAction)
            return false;

        command = std::move(*m_pendingResupplyAction);
        m_pendingResupplyAction.reset();
        return true;
    }

    void CompleteResupplyAction(
        const GTA_Resupply_Action_Command& command,
        GTA_Resupply_Action_Status status,
        std::string detail)
    {
        std::scoped_lock lock(m_mutex);
        if (m_resupplyAction.requestId != command.id)
            return;

        ++m_resupplyAction.revision;
        m_resupplyAction.status = status;
        m_resupplyAction.detail = std::move(detail);
    }

    [[nodiscard]] bool QueueSpecialCargoAction(GTA_Special_Cargo_Action_Command command)
    {
        std::scoped_lock lock(m_mutex);
        if (m_pendingSpecialCargoAction ||
            m_specialCargoAction.status == GTA_Special_Cargo_Action_Status::Queued) {
            return false;
        }

        command.id = m_nextSpecialCargoActionId++;
        m_pendingSpecialCargoAction = std::move(command);

        ++m_specialCargoAction.revision;
        m_specialCargoAction.requestId = m_pendingSpecialCargoAction->id;
        m_specialCargoAction.kind = m_pendingSpecialCargoAction->kind;
        m_specialCargoAction.warehouseSlot = m_pendingSpecialCargoAction->warehouseSlot;
        m_specialCargoAction.status = GTA_Special_Cargo_Action_Status::Queued;
        m_specialCargoAction.detail = "Queued for the GTA game thread.";
        return true;
    }

    [[nodiscard]] bool ConsumeSpecialCargoAction(GTA_Special_Cargo_Action_Command& command)
    {
        std::scoped_lock lock(m_mutex);
        if (!m_pendingSpecialCargoAction)
            return false;

        command = std::move(*m_pendingSpecialCargoAction);
        m_pendingSpecialCargoAction.reset();
        return true;
    }

    void CompleteSpecialCargoAction(
        const GTA_Special_Cargo_Action_Command& command,
        GTA_Special_Cargo_Action_Status status,
        std::string detail)
    {
        std::scoped_lock lock(m_mutex);
        if (m_specialCargoAction.requestId != command.id)
            return;

        ++m_specialCargoAction.revision;
        m_specialCargoAction.status = status;
        m_specialCargoAction.detail = std::move(detail);
    }

    [[nodiscard]] bool QueueVehicleCargoAction(GTA_Vehicle_Cargo_Action_Command command)
    {
        std::scoped_lock lock(m_mutex);
        if (m_pendingVehicleCargoAction ||
            m_vehicleCargoAction.status == GTA_Vehicle_Cargo_Action_Status::Queued) {
            return false;
        }

        command.id = m_nextVehicleCargoActionId++;
        m_pendingVehicleCargoAction = std::move(command);

        ++m_vehicleCargoAction.revision;
        m_vehicleCargoAction.requestId = m_pendingVehicleCargoAction->id;
        m_vehicleCargoAction.kind = m_pendingVehicleCargoAction->kind;
        m_vehicleCargoAction.status = GTA_Vehicle_Cargo_Action_Status::Queued;
        m_vehicleCargoAction.detail = "Queued for the GTA game thread.";
        return true;
    }

    [[nodiscard]] bool ConsumeVehicleCargoAction(GTA_Vehicle_Cargo_Action_Command& command)
    {
        std::scoped_lock lock(m_mutex);
        if (!m_pendingVehicleCargoAction)
            return false;

        command = std::move(*m_pendingVehicleCargoAction);
        m_pendingVehicleCargoAction.reset();
        return true;
    }

    void CompleteVehicleCargoAction(
        const GTA_Vehicle_Cargo_Action_Command& command,
        GTA_Vehicle_Cargo_Action_Status status,
        std::string detail)
    {
        std::scoped_lock lock(m_mutex);
        if (m_vehicleCargoAction.requestId != command.id)
            return;

        ++m_vehicleCargoAction.revision;
        m_vehicleCargoAction.status = status;
        m_vehicleCargoAction.detail = std::move(detail);
    }

    friend void TickBusinessExtension(GTA_Native_Manager& natives) noexcept;

    mutable std::mutex m_mutex;
    GTA_Nightclub_Snapshot m_nightclub{};
    std::optional<GTA_Nightclub_Action_Command> m_pendingNightclubAction;
    GTA_Nightclub_Action_Snapshot m_nightclubAction{};
    std::uint64_t m_nextNightclubActionId = 1;
    std::optional<GTA_Resupply_Action_Command> m_pendingResupplyAction;
    GTA_Resupply_Action_Snapshot m_resupplyAction{};
    std::uint64_t m_nextResupplyActionId = 1;
    GTA_Special_Cargo_Snapshot m_specialCargo{};
    std::optional<GTA_Special_Cargo_Action_Command> m_pendingSpecialCargoAction;
    GTA_Special_Cargo_Action_Snapshot m_specialCargoAction{};
    std::uint64_t m_nextSpecialCargoActionId = 1;
    GTA_Vehicle_Cargo_Snapshot m_vehicleCargo{};
    std::optional<GTA_Vehicle_Cargo_Action_Command> m_pendingVehicleCargoAction;
    GTA_Vehicle_Cargo_Action_Snapshot m_vehicleCargoAction{};
    std::uint64_t m_nextVehicleCargoActionId = 1;
};

[[nodiscard]] const char* GTA_Nightclub_Good_Name(GTA_Nightclub_Good good) noexcept;
[[nodiscard]] const char* GTA_Nightclub_Action_Name(GTA_Nightclub_Action_Kind kind) noexcept;
[[nodiscard]] const char* GTA_Nightclub_Action_Status_Name(GTA_Nightclub_Action_Status status) noexcept;
[[nodiscard]] const char* GTA_Resupply_Target_Name(GTA_Resupply_Target target) noexcept;
[[nodiscard]] const char* GTA_Resupply_Action_Name(GTA_Resupply_Action_Kind kind) noexcept;
[[nodiscard]] const char* GTA_Resupply_Action_Status_Name(GTA_Resupply_Action_Status status) noexcept;
}
