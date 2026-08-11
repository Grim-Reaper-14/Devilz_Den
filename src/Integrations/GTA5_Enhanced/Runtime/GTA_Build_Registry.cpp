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

std::size_t GTA_Build_Profile::LocatedRequiredTargetCount() const noexcept
{
    return static_cast<std::size_t>(std::count_if(targets.begin(), targets.end(), [](const auto& target) {
        return target.required && (target.state == GTA_Runtime_Target_State::Located ||
                                   target.state == GTA_Runtime_Target_State::StructurallyValidated ||
                                   target.state == GTA_Runtime_Target_State::Validated);
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
        {GTA_Runtime_Target_Id::GameState, GTA_Runtime_Target_State::Unknown, false, "GameState", "Legacy optional target; no verified locator is required by the active DLL runtime"},
        {GTA_Runtime_Target_Id::FrameCount, GTA_Runtime_Target_State::Unknown, false, "FrameCount", "Legacy optional target; no verified locator is required by the active DLL runtime"},
        {GTA_Runtime_Target_Id::ScriptGlobals, GTA_Runtime_Target_State::Unknown, false, "ScriptGlobals", "Optional script-global capability; semantic identity is build-specific"},
        {GTA_Runtime_Target_Id::ProgramTable, GTA_Runtime_Target_State::Unknown, false, "ProgramTable", "Optional script-program capability; semantic identity is build-specific"},
        {GTA_Runtime_Target_Id::ScriptThreads, GTA_Runtime_Target_State::Unknown, true, "ScriptThreads", "Required game-thread script context locator"},
        {GTA_Runtime_Target_Id::RunScriptThreads, GTA_Runtime_Target_State::Unknown, true, "RunScriptThreads", "Required game-thread execution bridge entry"},
        {GTA_Runtime_Target_Id::InitNativeTables, GTA_Runtime_Target_State::Unknown, true, "InitNativeTables", "Required Enhanced native bootstrap entry"},
        {GTA_Runtime_Target_Id::ScriptVM, GTA_Runtime_Target_State::Unknown, false, "ScriptVM", "Optional script bytecode execution capability used by network session transitions"},
        {GTA_Runtime_Target_Id::NativeTable, GTA_Runtime_Target_State::Unknown, false, "NativeTable", "Standalone native table is not required by the Enhanced bootstrap path"}
    };
    Register(std::move(current));
}

void GTA_Build_Registry::RecalculateVerification(GTA_Build_Profile& profile) noexcept
{
    const auto required = profile.RequiredTargetCount();
    const auto located = profile.LocatedRequiredTargetCount();
    const auto validated = profile.ValidatedRequiredTargetCount();

    if (required == 0) {
        profile.verification = GTA_Build_Verification_State::Known;
    } else if (validated == required) {
        profile.verification = GTA_Build_Verification_State::Supported;
    } else if (located != 0 || validated != 0) {
        profile.verification = GTA_Build_Verification_State::PartiallyVerified;
    } else {
        profile.verification = GTA_Build_Verification_State::RuntimeUnverified;
    }
}

void GTA_Build_Registry::Register(GTA_Build_Profile profile)
{
    if (profile.fingerprint == 0)
        return;
    RecalculateVerification(profile);
    m_profiles.insert_or_assign(profile.fingerprint, std::move(profile));
}

bool GTA_Build_Registry::ApplyTargetStatuses(
    std::uint64_t fingerprint,
    const std::vector<GTA_Runtime_Target_Status>& statuses)
{
    const auto it = m_profiles.find(fingerprint);
    if (it == m_profiles.end())
        return false;

    auto& profile = it->second;
    for (const auto& incoming : statuses) {
        const auto target = std::find_if(profile.targets.begin(), profile.targets.end(), [&](const auto& existing) {
            return existing.id == incoming.id;
        });
        if (target != profile.targets.end())
            *target = incoming;
    }

    RecalculateVerification(profile);
    return true;
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
