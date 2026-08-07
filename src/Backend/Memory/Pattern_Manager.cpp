#include "Pattern_Manager.hpp"

#include "Backend/Logging/Logger_Manager.hpp"

#include <future>
#include <memory>

namespace Devilz::Backend
{
Pattern_Manager::Pattern_Manager(ThreadManager& threads, File_System_Manager& files, std::filesystem::path cachePath)
    : m_threads(threads), m_files(files), m_cachePath(std::move(cachePath)), m_persistence(&files)
{
}

Result<void> Pattern_Manager::Initialize()
{
    m_state.store(ServiceState::Initializing);
    auto main = m_modules.MainModule();
    if (!main) {
        m_state.store(ServiceState::Failed);
        return Result<void>::Failure(main.Failure());
    }

    auto loaded = m_persistence.LoadForModule(m_cache, main.Value(), m_cachePath);
    if (!loaded) {
        Logger_Manager::Instance().LogError(LogLevel::Warning, loaded.Failure(), "Pattern_Manager");
    } else if (loaded.Value() > 0) {
        Logger_Manager::Instance().Info("Loaded " + std::to_string(loaded.Value()) + " validated pattern cache entries", "Pattern_Manager");
    }

    m_state.store(ServiceState::Created);
    return Result<void>::Success();
}

Result<void> Pattern_Manager::Start()
{
    if (m_state.load() == ServiceState::Failed)
        return Result<void>::Failure(Error(ErrorCode::ServiceInitializationFailed, ErrorCategory::Service, "Pattern_Manager cannot start from failed state"));
    m_state.store(ServiceState::Running);
    return Result<void>::Success();
}

Result<void> Pattern_Manager::Stop()
{
    m_state.store(ServiceState::Stopping);
    auto saved = PersistCache();
    m_state.store(ServiceState::Stopped);
    return saved;
}

void Pattern_Manager::Shutdown() noexcept
{
    auto saved = PersistCache();
    if (!saved) Logger_Manager::Instance().LogError(LogLevel::Error, saved.Failure(), "Pattern_Manager");
    m_state.store(ServiceState::Stopped);
}

Result<void> Pattern_Manager::PersistCache()
{
    auto result = m_persistence.Save(m_cache, m_cachePath);
    if (!result) Logger_Manager::Instance().LogError(LogLevel::Error, result.Failure(), "Pattern_Manager");
    return result;
}

Result<std::vector<Pattern_Resolution>> Pattern_Manager::ResolveMain(const std::vector<Pattern_Request>& requests)
{
    auto module = m_modules.MainModule();
    if (!module) return Result<std::vector<Pattern_Resolution>>::Failure(module.Failure());
    return ResolveParallel(module.Value(), requests);
}

Result<std::vector<Pattern_Resolution>> Pattern_Manager::ResolveModule(
    std::string_view moduleName, const std::vector<Pattern_Request>& requests)
{
    auto module = m_modules.FindLoaded(moduleName);
    if (!module) return Result<std::vector<Pattern_Resolution>>::Failure(module.Failure());

    auto loaded = m_persistence.LoadForModule(m_cache, module.Value(), m_cachePath);
    if (!loaded) Logger_Manager::Instance().LogError(LogLevel::Warning, loaded.Failure(), "Pattern_Manager");
    return ResolveParallel(module.Value(), requests);
}

Result<std::vector<Pattern_Resolution>> Pattern_Manager::ResolveParallel(
    const Module_Info& module, const std::vector<Pattern_Request>& requests)
{
    if (requests.empty()) return Result<std::vector<Pattern_Resolution>>::Success({});

    using JobResult = Result<Pattern_Resolution>;
    std::vector<std::future<JobResult>> futures;
    futures.reserve(requests.size());

    for (const auto& request : requests) {
        auto promise = std::make_shared<std::promise<JobResult>>();
        futures.push_back(promise->get_future());
        const Module_Info moduleCopy = module;
        const Pattern_Request requestCopy = request;

        m_threads.Workers().Submit([this, promise, moduleCopy, requestCopy]() mutable {
            try {
            {
                Pattern_Batch_Scanner scanner(&m_cache);
                auto result = scanner.Resolve(moduleCopy, std::vector<Pattern_Request>{requestCopy});
                if (!result) {
                    promise->set_value(JobResult::Failure(result.Failure()));
                    return;
                }
                promise->set_value(JobResult::Success(std::move(result.Value().front())));
            }
            } catch (const std::exception& exception) {
                promise->set_value(JobResult::Failure(
                    Error(ErrorCode::TaskExecutionFailed, ErrorCategory::Threading, "Pattern worker threw an exception")
                        .With("Pattern", requestCopy.name)
                        .With("Exception", exception.what())));
            } catch (...) {
                promise->set_value(JobResult::Failure(
                    Error(ErrorCode::TaskExecutionFailed, ErrorCategory::Threading, "Pattern worker threw an unknown exception")
                        .With("Pattern", requestCopy.name)));
            }
        });
    }

    std::vector<Pattern_Resolution> resolutions;
    resolutions.reserve(requests.size());
    for (std::size_t i = 0; i < futures.size(); ++i) {
        auto result = futures[i].get();
        if (!result) {
            Logger_Manager::Instance().LogError(LogLevel::Error, result.Failure(), "Pattern_Manager");
            return Result<std::vector<Pattern_Resolution>>::Failure(result.Failure());
        }
        resolutions.push_back(std::move(result.Value()));
    }

    auto saved = PersistCache();
    if (!saved) Logger_Manager::Instance().LogError(LogLevel::Warning, saved.Failure(), "Pattern_Manager");
    return Result<std::vector<Pattern_Resolution>>::Success(std::move(resolutions));
}
}
