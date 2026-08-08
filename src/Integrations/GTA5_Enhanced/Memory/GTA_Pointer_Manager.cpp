#include "GTA_Pointer_Manager.hpp"

#include "Backend/Logging/Logger_Manager.hpp"

#include <mutex>

namespace Devilz::Integrations::GTA5_Enhanced
{
using namespace Devilz::Backend;

Result<GTA_Pointers> GTA_Pointer_Manager::Resolve(const Build_Info& build, std::string_view moduleName)
{
    if (build.fingerprint == 0)
        return Result<GTA_Pointers>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
            "Cannot resolve GTA pointers without a build fingerprint"));

    const auto set = GTA_Patterns::Core(build);
    if (set.buildFingerprint != build.fingerprint || set.requests.empty())
        return Result<GTA_Pointers>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
            "No verified GTA Enhanced pattern set is registered for this build")
            .With("BuildFingerprint", std::to_string(build.fingerprint)));

    auto resolved = m_patterns.ResolveModule(moduleName, set.requests);
    if (!resolved) return Result<GTA_Pointers>::Failure(resolved.Failure());

    GTA_Pointers pointers{};
    auto assigned = Assign(resolved.Value(), pointers);
    if (!assigned) return Result<GTA_Pointers>::Failure(assigned.Failure());

    auto valid = Validate(pointers);
    if (!valid) return Result<GTA_Pointers>::Failure(valid.Failure());

    {
        std::unique_lock lock(m_mutex);
        m_pointers = pointers;
        m_buildFingerprint = build.fingerprint;
    }
    Logger_Manager::Instance().Info("Resolved and validated GTA Enhanced core pointers", "GTA.Pointer_Manager");
    return Result<GTA_Pointers>::Success(pointers);
}

Result<void> GTA_Pointer_Manager::Assign(const std::vector<Pattern_Resolution>& resolutions, GTA_Pointers& pointers) const
{
    for (const auto& resolution : resolutions) {
        if (resolution.name == "NativeTable") pointers.nativeTable = resolution.address;
        else if (resolution.name == "GameState") pointers.gameState = resolution.address;
        else if (resolution.name == "ScriptGlobals") pointers.scriptGlobals = resolution.address;
        else if (resolution.name == "FrameCount") pointers.frameCount = resolution.address;
    }
    return Result<void>::Success();
}

Result<void> GTA_Pointer_Manager::Validate(const GTA_Pointers& pointers) const
{
    if (pointers.nativeTable.IsNull())
        return Result<void>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime, "NativeTable pointer was not resolved"));
    if (pointers.gameState.IsNull())
        return Result<void>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime, "GameState pointer was not resolved"));
    return Result<void>::Success();
}

GTA_Pointers GTA_Pointer_Manager::Snapshot() const
{
    std::shared_lock lock(m_mutex);
    return m_pointers;
}

bool GTA_Pointer_Manager::Ready() const noexcept
{
    std::shared_lock lock(m_mutex);
    return m_buildFingerprint != 0 && m_pointers.Ready();
}

void GTA_Pointer_Manager::Clear() noexcept
{
    std::unique_lock lock(m_mutex);
    m_pointers = {};
    m_buildFingerprint = 0;
}
}
