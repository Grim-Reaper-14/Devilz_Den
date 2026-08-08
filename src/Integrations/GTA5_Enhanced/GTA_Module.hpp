#pragma once

#include "Backend/Filesystem/File_System_Manager.hpp"
#include "Backend/Memory/Pattern_Manager.hpp"
#include "Backend/Services/IBackendService.hpp"
#include "Diagnostics/GTA_Diagnostics.hpp"
#include "Memory/GTA_Pointer_Manager.hpp"
#include "Runtime/GTA_Runtime.hpp"

#include <atomic>
#include <filesystem>
#include <string_view>

namespace Devilz::Integrations::GTA5_Enhanced
{
class GTA_Module final : public Devilz::Backend::IBackendService
{
public:
    GTA_Module(Devilz::Backend::Pattern_Manager& patterns,
               Devilz::Backend::File_System_Manager& files,
               Build_Info build,
               std::filesystem::path crossmapPath);

    [[nodiscard]] std::string_view Name() const noexcept override { return "GTA5_Enhanced"; }
    [[nodiscard]] Devilz::Backend::ServiceState State() const noexcept override { return m_state.load(); }

    Devilz::Backend::Result<void> Initialize() override;
    Devilz::Backend::Result<void> Start() override;
    Devilz::Backend::Result<void> Stop() override;
    void Shutdown() noexcept override;

    [[nodiscard]] GTA_Runtime& Runtime() noexcept { return m_runtime; }
    [[nodiscard]] GTA_Diagnostics& Diagnostics() noexcept { return m_diagnostics; }

private:
    Devilz::Backend::Pattern_Manager& m_patterns;
    Devilz::Backend::File_System_Manager& m_files;
    Build_Info m_build;
    std::filesystem::path m_crossmapPath;
    GTA_Pointer_Manager m_pointerManager;
    GTA_Runtime m_runtime;
    GTA_Diagnostics m_diagnostics;
    std::atomic<Devilz::Backend::ServiceState> m_state{Devilz::Backend::ServiceState::Created};
};
}
