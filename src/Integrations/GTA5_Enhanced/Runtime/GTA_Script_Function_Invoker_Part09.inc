
    auto result = Execute(runtime, *request);
    CountCompletion(result);
    LogCompletion(runtime, result);

    try {
        std::scoped_lock lock(g_stateMutex);
        StoreSnapshotLocked(std::move(result));
    } catch (...) {
    }
    return true;
}
}
