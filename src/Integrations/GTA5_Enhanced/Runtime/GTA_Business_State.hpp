#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>
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

struct GTA_Nightclub_Good_Snapshot
{
    bool available = false;
    int units = 0;
    int maxUnits = 0;
    int unitValue = 0;
    int specialOrderUnitValue = 0;
    int productionTimeMs = 0;
};

struct GTA_Nightclub_Snapshot
{
    bool runtimeReady = false;
    bool owned = false;
    int playerIndex = -1;
    int nightclubIndex = 0;
    float popularity = 0.0F;
    int safeCash = 0;
    float equipmentUpgradeMultiplier = 1.0F;
    int managementMissionCooldownMs = 0;
    int sellMissionCooldownMs = 0;
    int specialOrderSellCooldownMs = 0;
    std::array<GTA_Nightclub_Good_Snapshot, GTA_Nightclub_Good_Count> goods{};
    std::array<int, 20> popularityIncome{};
    std::string detail = "Nightclub runtime is unavailable.";
};

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

    void PublishNightclub(GTA_Nightclub_Snapshot snapshot)
    {
        std::scoped_lock lock(m_mutex);
        m_nightclub = std::move(snapshot);
    }

    void Reset()
    {
        std::scoped_lock lock(m_mutex);
        m_nightclub = {};
    }

private:
    GTA_Business_State() = default;

    mutable std::mutex m_mutex;
    GTA_Nightclub_Snapshot m_nightclub{};
};

[[nodiscard]] const char* GTA_Nightclub_Good_Name(GTA_Nightclub_Good good) noexcept;
}
