#include "Native_Invoker.hpp"

namespace Devilz::Integrations::GTA5_Enhanced
{
using namespace Devilz::Backend;

Result<void> Native_Invoker::InvokeResolved(Pointer handler, Native_Call_Context& context, Native_Hash canonicalHash)
{
    // Handler resolution is now separated from the call ABI. This keeps the fast indexed
    // path usable as soon as the current-build scrNativeCallContext adapter is verified.
    ++m_stats.failures;
    return Result<void>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
        "Native handler resolved, but no verified call ABI adapter is installed")
        .With("CanonicalHash", std::to_string(canonicalHash.Value()))
        .With("Handler", std::to_string(handler.Address()))
        .With("ArgumentCount", std::to_string(context.ArgumentCount())));
}

Result<void> Native_Invoker::Invoke(Native_Index index, Native_Call_Context& context)
{
    ++m_stats.calls;
    if (!m_manager.Ready()) {
        ++m_stats.failures;
        return Result<void>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
            "Indexed native invocation requested before Native_Manager is ready"));
    }
    if (!m_indexedHandlers) {
        ++m_stats.failures;
        return Result<void>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
            "Indexed native invocation requested without a Native_Handler_Table"));
    }

    auto handler = m_indexedHandlers->Get(index);
    if (!handler) {
        ++m_stats.failures;
        return Result<void>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
            "Indexed native handler is unresolved or unvalidated")
            .With("NativeIndex", std::to_string(index.Value())));
    }

    ++m_stats.indexedHits;
    const auto runtimeHash = m_indexedHandlers->RuntimeHash(index).value_or(Native_Hash{});
    return InvokeResolved(*handler, context, runtimeHash);
}

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

    return InvokeResolved(handler, context, canonicalHash);
}
}
