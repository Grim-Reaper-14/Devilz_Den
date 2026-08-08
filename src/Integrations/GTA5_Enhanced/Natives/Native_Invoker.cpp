#include "Native_Invoker.hpp"

#include "Backend/Logging/Logger_Manager.hpp"

namespace Devilz::Integrations::GTA5_Enhanced
{
using namespace Devilz::Backend;

Result<void> Native_Invoker::Invoke(Native_Hash canonicalHash, Native_Call_Context& context)
{
    ++m_stats.calls;
    if (!m_manager.Ready()) {
        ++m_stats.failures;
        return Result<void>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
            "Native invocation requested before Native_Manager is ready"));
    }

    Pointer handler;
    if (auto cached = m_manager.FindCached(canonicalHash); cached && cached->validated) {
        handler = cached->handler;
        ++m_stats.cacheHits;
    } else {
        auto translated = m_manager.Translate(canonicalHash);
        if (!translated) { ++m_stats.failures; return Result<void>::Failure(translated.Failure()); }
        auto registration = m_table.Find(translated.Value());
        if (!registration || !registration->validated || registration->handler.IsNull()) {
            ++m_stats.failures;
            return Result<void>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
                "Native handler is unresolved or unvalidated")
                .With("CanonicalHash", std::to_string(canonicalHash.Value()))
                .With("RuntimeHash", std::to_string(translated.Value().Value())));
        }
        handler = registration->handler;
        m_manager.CacheResolved(canonicalHash, handler, true);
        ++m_stats.tableHits;
    }

    // Invocation ABI is intentionally isolated here. The integration must provide a
    // verified current-build adapter before arbitrary handler execution is enabled.
    // Until then, resolution/validation is usable while calls fail closed.
    ++m_stats.failures;
    return Result<void>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
        "Native handler resolved, but no verified call ABI adapter is installed")
        .With("CanonicalHash", std::to_string(canonicalHash.Value()))
        .With("Handler", std::to_string(handler.Address()))
        .With("ArgumentCount", std::to_string(context.ArgumentCount())));
}
}
