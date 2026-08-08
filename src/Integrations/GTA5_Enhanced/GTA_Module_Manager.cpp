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

    if (!IsSupportedBuild(*m_status.build)) {
        m_status.state = GTA_Module_Manager_State::Unsupported;
        m_status.detail = "No verified runtime pattern set is registered for this GTA Enhanced build";
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

bool GTA_Module_Manager::IsSupportedBuild(const Build_Info& build) noexcept
{
    // Fail closed until a build fingerprint has a verified pattern definition.
    // This prevents accidental runtime initialization against guessed offsets.
    (void)build;
    return false;
}
}
