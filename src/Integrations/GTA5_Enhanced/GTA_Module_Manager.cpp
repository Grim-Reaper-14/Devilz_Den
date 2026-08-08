#include "GTA_Module_Manager.hpp"

namespace Devilz::Integrations::GTA5_Enhanced
{
using namespace Devilz::Backend;

Result<GTA_Module_Status> GTA_Module_Manager::Refresh()
{
    m_status = {};

    auto process = m_processes.Find("GTA5_Enhanced.exe");
    if (!process) {
        m_status.state = GTA_Module_Manager_State::NotRunning;
        m_status.detail = "GTA5_Enhanced.exe is not running";
        return Result<GTA_Module_Status>::Success(m_status);
    }

    m_status.process = process.Value();
    m_status.state = GTA_Module_Manager_State::Detected;
    m_status.detail = "GTA5_Enhanced.exe detected";

    auto build = m_buildDetector.Detect(*m_status.process);
    if (!build) {
        m_status.state = GTA_Module_Manager_State::Failed;
        m_status.detail = build.Failure().Message();
        return Result<GTA_Module_Status>::Success(m_status);
    }

    m_status.build = build.Value();
    m_status.state = GTA_Module_Manager_State::BuildIdentified;
    m_status.detail = "GTA Enhanced build identified";

    const auto profile = m_buildRegistry.Find(m_status.build->fingerprint);
    if (!profile) {
        m_status.state = GTA_Module_Manager_State::Unsupported;
        m_status.detail = "GTA Enhanced build is not registered";
        return Result<GTA_Module_Status>::Success(m_status);
    }

    m_status.profile = *profile;
    if (!profile->RuntimeSupported()) {
        m_status.state = GTA_Module_Manager_State::RuntimeUnverified;
        m_status.detail = "Build is known, but its runtime targets have not been fully verified";
        return Result<GTA_Module_Status>::Success(m_status);
    }

    m_status.state = GTA_Module_Manager_State::Supported;
    m_status.detail = "GTA Enhanced build is supported";
    return Result<GTA_Module_Status>::Success(m_status);
}

GTA_Module_Status GTA_Module_Manager::Snapshot() const
{
    return m_status;
}
}
