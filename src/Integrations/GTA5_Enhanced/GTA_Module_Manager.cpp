#include "GTA_Module_Manager.hpp"

#include <algorithm>
#include <cctype>

namespace Devilz::Integrations::GTA5_Enhanced
{
using namespace Devilz::Backend;

namespace
{
bool EqualInsensitive(std::string_view left, std::string_view right)
{
    if (left.size() != right.size()) return false;
    return std::equal(left.begin(), left.end(), right.begin(), right.end(), [](char a, char b) {
        return std::tolower(static_cast<unsigned char>(a)) ==
               std::tolower(static_cast<unsigned char>(b));
    });
}
}

Result<GTA_Module_Status> GTA_Module_Manager::Refresh()
{
    m_status = {};

    auto process = m_processes.Current();
    if (!process) {
        m_status.state = GTA_Module_Manager_State::Failed;
        m_status.detail = "Unable to inspect the current host process: " + process.Failure().Message();
        return Result<GTA_Module_Status>::Success(m_status);
    }

    if (!EqualInsensitive(process.Value().executableName, "GTA5_Enhanced.exe")) {
        m_status.state = GTA_Module_Manager_State::NotRunning;
        m_status.detail = "Devilz_Den is not loaded inside GTA5_Enhanced.exe; current host is " +
                          process.Value().executableName;
        return Result<GTA_Module_Status>::Success(m_status);
    }

    m_status.process = process.Value();
    m_status.state = GTA_Module_Manager_State::Detected;
    m_status.detail = "GTA5_Enhanced.exe detected as the current host process";

    auto build = m_buildDetector.Detect(*m_status.process);
    if (!build) {
        m_status.state = GTA_Module_Manager_State::Failed;
        m_status.detail = build.Failure().Message();
        return Result<GTA_Module_Status>::Success(m_status);
    }

    m_status.build = build.Value();
    m_status.state = GTA_Module_Manager_State::BuildIdentified;
    m_status.detail = "GTA Enhanced build identified";

    if (!m_buildRegistry.Known(m_status.build->fingerprint)) {
        m_status.state = GTA_Module_Manager_State::Unsupported;
        m_status.detail = "GTA Enhanced build is not registered";
        return Result<GTA_Module_Status>::Success(m_status);
    }

    auto targets = m_targetCoordinator.Resolve(m_status.process->pid, m_status.build->fingerprint);
    if (!targets) {
        m_status.state = GTA_Module_Manager_State::Failed;
        m_status.detail = "Runtime target discovery failed: " + targets.Failure().Message();
        return Result<GTA_Module_Status>::Success(m_status);
    }
    m_status.targetReport = std::move(targets.Value());

    std::vector<GTA_Runtime_Target_Status> statuses;
    statuses.reserve(m_status.targetReport->targets.size());
    for (const auto& target : m_status.targetReport->targets)
        statuses.push_back(target.status);

    if (!m_buildRegistry.ApplyTargetStatuses(m_status.build->fingerprint, statuses)) {
        m_status.state = GTA_Module_Manager_State::Failed;
        m_status.detail = "Unable to apply runtime target results to the GTA build profile";
        return Result<GTA_Module_Status>::Success(m_status);
    }

    m_status.profile = m_buildRegistry.Find(m_status.build->fingerprint);
    if (!m_status.profile) {
        m_status.state = GTA_Module_Manager_State::Failed;
        m_status.detail = "GTA build profile disappeared after runtime target discovery";
        return Result<GTA_Module_Status>::Success(m_status);
    }

    switch (m_status.profile->verification) {
    case GTA_Build_Verification_State::Supported:
        m_status.state = GTA_Module_Manager_State::Supported;
        m_status.detail = "All required GTA runtime targets are semantically validated";
        break;
    case GTA_Build_Verification_State::PartiallyVerified:
        m_status.state = GTA_Module_Manager_State::RuntimeUnverified;
        m_status.detail = "Some GTA runtime targets are located or structurally validated; semantic verification is still pending";
        break;
    default:
        m_status.state = GTA_Module_Manager_State::RuntimeUnverified;
        m_status.detail = "Build is known; required GTA runtime targets are not yet verified";
        break;
    }

    return Result<GTA_Module_Status>::Success(m_status);
}

GTA_Module_Status GTA_Module_Manager::Snapshot() const
{
    return m_status;
}
}
