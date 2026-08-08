#pragma once

#include "Backend/Error/Result.hpp"
#include "Globals/Script_Global_Manager.hpp"
#include "Script_VM.hpp"
#include "scrProgramTable.hpp"
#include "../Runtime/Build_Info.hpp"

#include <atomic>
#include <cstdint>

namespace Devilz::Integrations::GTA5_Enhanced
{
enum class Script_Runtime_State : std::uint8_t
{
    Created,
    Initializing,
    Ready,
    Failed,
    Stopped
};

class Script_Runtime_Manager final
{
public:
    Devilz::Backend::Result<void> Initialize(const Build_Info& build);
    void Shutdown() noexcept;

    [[nodiscard]] scrProgramTable& Programs() noexcept { return m_programs; }
    [[nodiscard]] Script_VM& VM() noexcept { return m_vm; }
    [[nodiscard]] Script_Global_Manager& Globals() noexcept { return m_globals; }
    [[nodiscard]] const Build_Info& Build() const noexcept { return m_build; }
    [[nodiscard]] Script_Runtime_State State() const noexcept { return m_state.load(); }
    [[nodiscard]] bool Ready() const noexcept { return State() == Script_Runtime_State::Ready; }

private:
    Build_Info m_build{};
    scrProgramTable m_programs;
    Script_VM m_vm;
    Script_Global_Manager m_globals;
    std::atomic<Script_Runtime_State> m_state{Script_Runtime_State::Created};
};
}
