#include "GTA_Build_Registry.hpp"

#include <algorithm>
#include <utility>

namespace Devilz::Integrations::GTA5_Enhanced
{
std::size_t GTA_Build_Profile::RequiredTargetCount() const noexcept
{
    return static_cast<std::size_t>(std::count_if(targets.begin(), targets.end(), [](const auto& target) {
        return target.required;
    }));
}

std::size_t GTA_Build_Profile::ValidatedRequiredTargetCount() const noexcept
{
    return static_cast<std::size_t>(std::count_if(targets.begin(), targets.end(), [](const auto& target) {
        return target.required && target.state == GTA_Runtime_Target_State::Validated;
    }));
}

bool GTA_Build_Profile::AllRequiredTargetsValidated() const noexcept
{
    const auto required = RequiredTargetCount();
    return required != 0 && ValidatedRequiredTargetCount() == required;
}

GTA_Build_Registry::GTA_Build_Registry()
{
    GTA_Build_Profile current{};
    current.fingerprint = 0x6A4F97F605B81000ULL;
    current.label = "PE-1783601142-95948800";
    current.targets = {
        {GTA_Runtime_Target_Id::GameState, GTA_Runtime_Target_State::Unknown, true, "GameState", "Awaiting verified locator"},
        {GTA_Runtime_Target_Id::FrameCount, GTA_Runtime_Target_State::Unknown, true, "FrameCount", "Awaiting verified locator"},
        {GTA_Runtime_Target_Id::ScriptGlobals, GTA_Runtime_Target_State::Unknown, true, "ScriptGlobals", "Awaiting verified locator"},
        {GTA_Runtime_Target_Id::ProgramTable, GTA_Runtime_Target_State::Unknown, true, "ProgramTable", "Awaiting verified locator"},
        {GTA_Runtime_Target_Id::ScriptThreads, GTA_Runtime_Target_State::Unknown, true, "ScriptThreads", "Awaiting verified locator"},
        {GTA_Runtime_Target_Id::InitNativeTables, GTA_Runtime_Target_State::Unknown, true, "InitNativeTables", "Awaiting verified locator"},
        {GTA_Runtime_Target_Id::NativeTable, GTA_Runtime_Target_State::Unknown, true, "NativeTable", "Awaiting verified locator"}
    };
    Register(std::move(current));
}

void GTA_Build_Registry::RecalculateVerification(GTA_Build_Profile& profile) noexcept
{
    const auto required = profile.RequiredTargetCount();
    const auto validated = profile.ValidatedRequiredTargetCount();

    if (required == 0) {
        profile.verification = GTA_Build_Verification_State::Known;
    } else if (validated == 0) {
        profile.verification = GTA_Build_Verification_State::RuntimeUnverified;
    } else if (validated < required) {
        profile.verification = GTA_Build_Verification_State::PartiallyVerified;
    } else {
        profile.verification = GTA_Build_Verification_State::Supported;
    }
}

void GTA_Build_Registry::Register(GTA_Build_Profile profile)
{
    if (profile.fingerprint == 0)
        return;
    RecalculateVerification(profile);
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
