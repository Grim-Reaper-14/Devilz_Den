#include "ThreadManager.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace Devilz::Backend
{
QueueExecutor::QueueExecutor(std::string name, std::size_t workerCount) : m_name(std::move(name))
{
    workerCount = std::max<std::size_t>(1, workerCount);
    m_workers.reserve(workerCount);
    for (std::size_t i = 0; i < workerCount; ++i)
        m_workers.emplace_back([this, i](std::stop_token token) { Worker(token, i); });
}

QueueExecutor::~QueueExecutor() { Stop(); }

TaskId QueueExecutor::Submit(Task task)
{
    if (!task) throw std::invalid_argument("Cannot submit an empty task");
    const TaskId id = ++m_nextTask;
    {
        std::scoped_lock lock(m_mutex);
        if (!m_accepting) throw std::runtime_error("Executor is stopped");
        m_queue.push_back({id, std::move(task)});
    }
    m_cv.notify_one();
    return id;
}

std::size_t QueueExecutor::Pending() const noexcept
{
    std::scoped_lock lock(m_mutex);
    return m_queue.size();
}

void QueueExecutor::Stop()
{
    {
        std::scoped_lock lock(m_mutex);
        if (!m_accepting && m_workers.empty()) return;
        m_accepting = false;
    }
    m_cv.notify_all();
    for (auto& worker : m_workers) worker.request_stop();
    m_workers.clear();
}

void QueueExecutor::Worker(std::stop_token token, std::size_t)
{
    while (!token.stop_requested()) {
        WorkItem item{};
        {
            std::unique_lock lock(m_mutex);
            m_cv.wait(lock, token, [this] { return !m_queue.empty() || !m_accepting; });
            if (m_queue.empty()) {
                if (!m_accepting || token.stop_requested()) break;
                continue;
            }
            item = std::move(m_queue.front());
            m_queue.pop_front();
        }
        try { item.task(); }
        catch (...) { /* Logger integration follows in the next backend pass. */ }
    }
}

ThreadManager::ThreadManager() = default;
ThreadManager::~ThreadManager() { Stop(); }

void ThreadManager::Start()
{
    if (m_workers) return;
    const auto hardware = std::max(2u, std::thread::hardware_concurrency());
    m_workers = std::make_unique<QueueExecutor>("Workers", hardware - 1);
    m_io = std::make_unique<QueueExecutor>("IO", 2);
}

void ThreadManager::Stop()
{
    for (auto& executor : m_dedicated) executor->Stop();
    m_dedicated.clear();
    if (m_io) m_io->Stop();
    if (m_workers) m_workers->Stop();
    m_io.reset();
    m_workers.reset();
}

IExecutor& ThreadManager::Workers()
{
    if (!m_workers) throw std::logic_error("ThreadManager is not started");
    return *m_workers;
}

IExecutor& ThreadManager::IO()
{
    if (!m_io) throw std::logic_error("ThreadManager is not started");
    return *m_io;
}

IExecutor& ThreadManager::CreateDedicated(std::string name)
{
    auto executor = std::make_unique<QueueExecutor>(std::move(name), 1);
    auto& reference = *executor;
    m_dedicated.push_back(std::move(executor));
    return reference;
}
}
