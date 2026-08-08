#pragma once

#include "Backend/Error/Result.hpp"
#include "Backend/Filesystem/File_System_Manager.hpp"
#include "Build_Info.hpp"
#include "Game_State.hpp"
#include "../Memory/GTA_Pointers.hpp"
#include "../Natives/Native_Handler_Table.hpp"
#include "../Natives/Native_Invoker.hpp"
#include "../Natives/Native_Manager.hpp"
#include "../Natives/Native_Table.hpp"
#include "../Script/Script_Runtime_Manager.hpp"

#include <atomic>
#include <filesystem>

namespace Devilz::Integrations::GTA5_Enhanced
{
enum class GTA_Runtime_State : std::uint8_t
{
    Created,
    Initializing,
    Ready,
    Degraded,
    Failed,
    Stopped
};

class GTA_Runtime final
{
public:
    explicit GTA_Runtime(Devilz::Backend::File_System_Manager& files);

    Devilz::Backend::Result<void> Initialize(
        Build_Info build,
        GTA_Pointers pointers,
        const std::filesystem::path& crossmapPath);

    void Shutdown() noexcept;

    [[nodiscard]] GTA_Runtime_State State() const noexcept { return m_state.load(); }
    [[nodiscard]] bool Ready() const noexcept { return State() == GTA_Runtime_State::Ready; }

    [[nodiscard]] const Build_Info& Build() const noexcept { return m_build; }
    [[nodiscard]] const GTA_Pointers& Pointers() const noexcept { return m_pointers; }
    [[nodiscard]] Game_State_Type GameState() const noexcept { return m_gameState.load(); }
    void SetGameState(Game_State_Type state) noexcept { m_gameState.store(state); }

    [[nodiscard]] Script_Runtime_Manager& Scripts() noexcept { return m_scripts; }
    [[nodiscard]] Native_Manager& Natives() noexcept { return m_natives; }
    [[nodiscard]] Native_Table& NativeTable() noexcept { return m_nativeTable; }
    [[nodiscard]] Native_Handler_Table& NativeHandlers() noexcept { return m_nativeHandlers; }
    [[nodiscard]] Native_Invoker& Invoker() noexcept { return m_invoker; }

private:
    Devilz::Backend::Result<void> ValidatePointers() const;

    Devilz::Backend::File_System_Manager& m_files;
    Build_Info m_build{};
    GTA_Pointers m_pointers{};
    Script_Runtime_Manager m_scripts;
    Native_Manager m_natives;
    Native_Table m_nativeTable;
    Native_Handler_Table m_nativeHandlers;
    Native_Invoker m_invoker;
    std::atomic<GTA_Runtime_State> m_state{GTA_Runtime_State::Created};
    std::atomic<Game_State_Type> m_gameState{Game_State_Type::Unknown};
};
}
