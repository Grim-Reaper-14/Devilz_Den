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

    void PublishNightclub(GTA_Nightclub_Snapshot snapshot)
    {
        std::scoped_lock lock(m_mutex);
        m_nightclub = std::move(snapshot);
    }

    void Reset()
    {
        std::scoped_lock lock(m_mutex);
        m_nightclub = {};
        m_pendingNightclubAction.reset();
        m_nightclubAction = {};
        m_nextNightclubActionId = 1;
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

    friend void TickBusinessExtension(GTA_Native_Manager& natives) noexcept;

    mutable std::mutex m_mutex;
    GTA_Nightclub_Snapshot m_nightclub{};
    std::optional<GTA_Nightclub_Action_Command> m_pendingNightclubAction;
    GTA_Nightclub_Action_Snapshot m_nightclubAction{};
    std::uint64_t m_nextNightclubActionId = 1;
};

[[nodiscard]] const char* GTA_Nightclub_Good_Name(GTA_Nightclub_Good good) noexcept;
[[nodiscard]] const char* GTA_Nightclub_Action_Name(GTA_Nightclub_Action_Kind kind) noexcept;
[[nodiscard]] const char* GTA_Nightclub_Action_Status_Name(GTA_Nightclub_Action_Status status) noexcept;
}
