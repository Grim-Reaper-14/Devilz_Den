#pragma once

#include "Backend/Services/IBackendService.hpp"
#include "File_System_Manager.hpp"
#include "File_System_Monitor_Manager.hpp"
#include "Folder_System_Manager.hpp"

#include <atomic>

namespace Devilz::Backend
{
class Folder_System_Module final : public IBackendService
{
public:
    explicit Folder_System_Module(std::filesystem::path root = {});

    [[nodiscard]] std::string_view Name() const noexcept override { return "FolderSystem"; }
    [[nodiscard]] ServiceState State() const noexcept override { return m_state.load(); }

    Result<void> Initialize() override;
    Result<void> Start() override;
    Result<void> Stop() override;
    void Shutdown() noexcept override;

    [[nodiscard]] File_System_Manager& Files() noexcept { return m_files; }
    [[nodiscard]] Folder_System_Manager& Folders() noexcept { return m_folders; }
    [[nodiscard]] File_System_Monitor_Manager& Monitors() noexcept { return m_monitors; }

private:
    std::atomic<ServiceState> m_state{ServiceState::Created};
    File_System_Manager m_files;
    Folder_System_Manager m_folders;
    File_System_Monitor_Manager m_monitors;
};
}
