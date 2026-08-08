#pragma once

#include "Build_Info.hpp"

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
    std::vector<std::string> verifiedTargets;

    [[nodiscard]] bool RuntimeSupported() const noexcept
    {
        return verification == GTA_Build_Verification_State::Supported;
    }
};

class GTA_Build_Registry final
{
public:
    GTA_Build_Registry();

    [[nodiscard]] std::optional<GTA_Build_Profile> Find(std::uint64_t fingerprint) const;
    [[nodiscard]] bool Known(std::uint64_t fingerprint) const noexcept;
    [[nodiscard]] bool Supported(std::uint64_t fingerprint) const noexcept;

private:
    void Register(GTA_Build_Profile profile);

    std::unordered_map<std::uint64_t, GTA_Build_Profile> m_profiles;
};
}
