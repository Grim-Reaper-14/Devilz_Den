#pragma once

#include "Build_Info.hpp"
#include "GTA_Runtime_Target.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Devilz::Integrations::GTA5_Enhanced
{
enum class GTA_Build_Verification_State : std::uint8_t
{
    Unknown,
    Known,
    RuntimeUnverified,
    PartiallyVerified,
    Supported
};

struct GTA_Build_Profile
{
    std::uint64_t fingerprint = 0;
    std::string label;
    GTA_Build_Verification_State verification = GTA_Build_Verification_State::Unknown;
    std::vector<GTA_Runtime_Target_Status> targets;

    [[nodiscard]] bool RuntimeSupported() const noexcept
    {
        return verification == GTA_Build_Verification_State::Supported;
    }

    [[nodiscard]] std::size_t RequiredTargetCount() const noexcept;
    [[nodiscard]] std::size_t LocatedRequiredTargetCount() const noexcept;
    [[nodiscard]] std::size_t ValidatedRequiredTargetCount() const noexcept;
    [[nodiscard]] bool AllRequiredTargetsValidated() const noexcept;
};

class GTA_Build_Registry final
{
public:
    GTA_Build_Registry();

    [[nodiscard]] std::optional<GTA_Build_Profile> Find(std::uint64_t fingerprint) const;
    [[nodiscard]] bool Known(std::uint64_t fingerprint) const noexcept;
    [[nodiscard]] bool Supported(std::uint64_t fingerprint) const noexcept;

    [[nodiscard]] bool ApplyTargetStatuses(
        std::uint64_t fingerprint,
        const std::vector<GTA_Runtime_Target_Status>& statuses);

private:
    void Register(GTA_Build_Profile profile);
    static void RecalculateVerification(GTA_Build_Profile& profile) noexcept;

    std::unordered_map<std::uint64_t, GTA_Build_Profile> m_profiles;
};
}
