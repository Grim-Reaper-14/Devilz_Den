#include "GTA_Runtime.hpp"

#include "Backend/Logging/Logger_Manager.hpp"

namespace Devilz::Integrations::GTA5_Enhanced
{
using namespace Devilz::Backend;

GTA_Runtime::GTA_Runtime(File_System_Manager& files)
    : m_files(files),
      m_natives(files),
      m_invoker(m_natives, m_nativeTable, &m_nativeHandlers)
{
}

Result<void> GTA_Runtime::ValidatePointers() const
{
    if (!m_pointers.nativeTable)
        return Result<void>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
            "GTA native table pointer is unresolved"));
    if (!m_pointers.gameState)
        return Result<void>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
            "GTA game-state pointer is unresolved"));
    return Result<void>::Success();
}

Result<void> GTA_Runtime::Initialize(Build_Info build, GTA_Pointers pointers,
                                     const std::filesystem::path& crossmapPath)
{
    m_state.store(GTA_Runtime_State::Initializing);
    m_build = std::move(build);
    m_pointers = pointers;

    if (m_build.fingerprint == 0) {
        m_state.store(GTA_Runtime_State::Failed);
        return Result<void>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
            "GTA runtime requires a validated build fingerprint"));
    }

    auto pointerValidation = ValidatePointers();
    if (!pointerValidation) {
        m_state.store(GTA_Runtime_State::Failed);
        return pointerValidation;
    }

    auto scripts = m_scripts.Initialize(m_build);
    if (!scripts) {
        m_state.store(GTA_Runtime_State::Failed);
        return scripts;
    }

    auto natives = m_natives.Initialize(m_build, crossmapPath);
    if (!natives) {
        Logger_Manager::Instance().LogError(LogLevel::Warning, natives.Failure(), "GTA_Runtime");
        m_state.store(GTA_Runtime_State::Degraded);
        return Result<void>::Success();
    }

    m_state.store(GTA_Runtime_State::Ready);
    Logger_Manager::Instance().Info("GTA Enhanced runtime initialized", "GTA_Runtime");
    return Result<void>::Success();
}

void GTA_Runtime::Shutdown() noexcept
{
    m_state.store(GTA_Runtime_State::Stopped);
    m_gameState.store(Game_State_Type::Unknown);
    m_nativeHandlers.Clear();
    m_nativeTable.Clear();
    m_natives.Shutdown();
    m_scripts.Shutdown();
    m_pointers = {};
    m_build = {};
}
}
