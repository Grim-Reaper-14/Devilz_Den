#pragma once

#include <cstdint>
#include <string>

namespace Devilz::Integrations::GTA5_Enhanced
{
enum class GTA_Bunker_Instant_Sell_Status : std::uint8_t
{
    Idle,
    Queued,
    Succeeded,
    RuntimeUnavailable,
    UnsupportedBuild,
    ScriptNotRunning,
    LocalUnavailable,
    Failed
};

struct GTA_Bunker_Instant_Sell_Snapshot
{
    std::uint64_t revision = 0;
    std::uint64_t requestId = 0;
    GTA_Bunker_Instant_Sell_Status status = GTA_Bunker_Instant_Sell_Status::Idle;
    std::string detail;
};

void ConfigureBunkerExtension(
    std::uintptr_t scriptThreadsStorageAddress,
    std::uint64_t buildFingerprint) noexcept;

void ResetBunkerExtension() noexcept;

[[nodiscard]] bool RequestBunkerInstantSell() noexcept;
[[nodiscard]] GTA_Bunker_Instant_Sell_Snapshot BunkerInstantSellSnapshot();
[[nodiscard]] const char* GTA_Bunker_Instant_Sell_Status_Name(
    GTA_Bunker_Instant_Sell_Status status) noexcept;

void TickBunkerExtension() noexcept;
}
