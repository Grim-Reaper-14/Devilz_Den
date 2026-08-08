#include "Native_Manager.hpp"

#include "Backend/Logging/Logger_Manager.hpp"

namespace Devilz::Integrations::GTA5_Enhanced
{
using namespace Devilz::Backend;

Native_Manager::Native_Manager(File_System_Manager& files) noexcept : m_files(files) {}

Result<void> Native_Manager::Initialize(Build_Info build, const std::filesystem::path& crossmapPath)
{
    m_ready = false;
    m_cache.Clear();
    m_crossmap.Clear();
    m_build = std::move(build);

    if (m_build.fingerprint == 0)
        return Result<void>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
            "GTA build fingerprint is unavailable; refusing to load a crossmap"));

    auto loaded = m_crossmap.LoadText(m_files, crossmapPath, m_build.fingerprint);
    if (!loaded) {
        Logger_Manager::Instance().LogError(LogLevel::Error, loaded.Failure(), "GTA.Native_Manager");
        return loaded;
    }

    if (m_crossmap.Statistics().entries == 0)
        return Result<void>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
            "Crossmap contains no entries for the detected GTA build")
            .With("BuildFingerprint", std::to_string(m_build.fingerprint))
            .With("Crossmap", crossmapPath.string()));

    m_ready = true;
    Logger_Manager::Instance().Info(
        "Native manager initialized with " + std::to_string(m_crossmap.Statistics().entries) + " crossmap entries",
        "GTA.Native_Manager");
    return Result<void>::Success();
}

void Native_Manager::Shutdown() noexcept
{
    m_ready = false;
    m_cache.Clear();
    m_crossmap.Clear();
}

Result<Native_Hash> Native_Manager::Translate(Native_Hash canonical) const
{
    if (!m_ready)
        return Result<Native_Hash>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
            "Native manager is not ready"));

    auto translated = m_crossmap.ToRuntime(canonical, m_build.fingerprint);
    if (!translated)
        return Result<Native_Hash>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
            "Native hash is not present in the active crossmap")
            .With("CanonicalHash", std::to_string(canonical.Value()))
            .With("BuildFingerprint", std::to_string(m_build.fingerprint)));
    return Result<Native_Hash>::Success(*translated);
}

std::optional<Native_Registration> Native_Manager::FindCached(Native_Hash canonical) const
{
    auto runtime = Translate(canonical);
    if (!runtime) return std::nullopt;
    return m_cache.Find(runtime.Value());
}

void Native_Manager::CacheResolved(Native_Hash canonical, Pointer handler, bool validated)
{
    auto runtime = Translate(canonical);
    if (!runtime) {
        Logger_Manager::Instance().LogError(LogLevel::Warning, runtime.Failure(), "GTA.Native_Manager");
        return;
    }
    m_cache.Store({runtime.Value(), handler, validated, 0});
}

bool Native_Manager::Ready() const noexcept
{
    return m_ready;
}
}
