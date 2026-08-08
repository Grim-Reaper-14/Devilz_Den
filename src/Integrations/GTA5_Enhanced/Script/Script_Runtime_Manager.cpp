#include "Script_Runtime_Manager.hpp"

#include "Backend/Logging/Logger_Manager.hpp"

namespace Devilz::Integrations::GTA5_Enhanced
{
using namespace Devilz::Backend;

Result<void> Script_Runtime_Manager::Initialize(const Build_Info& build)
{
    m_state.store(Script_Runtime_State::Initializing);
    if (build.fingerprint == 0) {
        m_state.store(Script_Runtime_State::Failed);
        return Result<void>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
            "Script runtime requires a validated GTA build fingerprint"));
    }

    m_build = build;
    m_programs.Clear();
    m_vm.ClearThreads();
    m_globals.Clear();
    m_state.store(Script_Runtime_State::Ready);
    Logger_Manager::Instance().Info("GTA script runtime initialized", "GTA.Script_Runtime");
    return Result<void>::Success();
}

void Script_Runtime_Manager::Shutdown() noexcept
{
    m_vm.ClearThreads();
    m_programs.Clear();
    m_globals.Clear();
    m_build = {};
    m_state.store(Script_Runtime_State::Stopped);
}
}
