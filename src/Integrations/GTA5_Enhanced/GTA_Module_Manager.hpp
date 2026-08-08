#pragma once

#include "Backend/Error/Result.hpp"
#include "Backend/Process/Process_Manager.hpp"
#include "Runtime/Build_Info.hpp"
#include "Runtime/Build_Info_Detector.hpp"
#include "Runtime/GTA_Build_Registry.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace Devilz::Integrations::GTA5_Enhanced
{
enum class GTA_Module_Manager_State : std::uint8_t
{
    NotRunning,
    Detected,
    BuildIdentified,
    RuntimeUnverified,
    Unsupported,
    Supported,
    RuntimeReady,
    Failed
};

struct GTA_Module_Status
{
    GTA_Module_Manager_State state = GTA_Module_Manager_State::NotRunning;
    std::optional<Devilz::Backend::Process_Info> process;
    std::optional<Build_Info> build;
    std::optional<GTA_Build_Profile> profile;
    std::string detail;
};

class GTA_Module_Manager final
{
public:
    [[nodiscard]] Devilz::Backend::Result<GTA_Module_Status> Refresh();
    [[nodiscard]] GTA_Module_Status Snapshot() const;

private:
    Devilz::Backend::Process_Manager m_processes;
    Build_Info_Detector m_buildDetector;
    GTA_Build_Registry m_buildRegistry;
    GTA_Module_Status m_status{};
};
}
