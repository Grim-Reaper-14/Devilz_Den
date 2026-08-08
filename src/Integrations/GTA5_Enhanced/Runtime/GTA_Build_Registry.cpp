#include "GTA_Build_Registry.hpp"

#include <utility>

namespace Devilz::Integrations::GTA5_Enhanced
{
GTA_Build_Registry::GTA_Build_Registry()
{
    GTA_Build_Profile current{};
    current.fingerprint = 0x6A4F97F605B81000ULL;
    current.label = "PE-1783601142-95948800";
    current.verification = GTA_Build_Verification_State::RuntimeUnverified;
    Register(std::move(current));
}

void GTA_Build_Registry::Register(GTA_Build_Profile profile)
{
    if (profile.fingerprint == 0)
        return;
    m_profiles.insert_or_assign(profile.fingerprint, std::move(profile));
}

std::optional<GTA_Build_Profile> GTA_Build_Registry::Find(std::uint64_t fingerprint) const
{
    const auto it = m_profiles.find(fingerprint);
    if (it == m_profiles.end())
        return std::nullopt;
    return it->second;
}

bool GTA_Build_Registry::Known(std::uint64_t fingerprint) const noexcept
{
    return m_profiles.contains(fingerprint);
}

bool GTA_Build_Registry::Supported(std::uint64_t fingerprint) const noexcept
{
    const auto it = m_profiles.find(fingerprint);
    return it != m_profiles.end() && it->second.RuntimeSupported();
}
}
