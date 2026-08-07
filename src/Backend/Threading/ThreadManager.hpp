#pragma once

#include "IExecutor.hpp"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace Devilz::Backend
{
class QueueExecutor final : public IExecutor
{
public:
    QueueExecutor(std::string name, std::size_t workerCount);
    ~QueueExecutor() override;

    std::string_view Name() const noexcept override { return m_name; }
    TaskId Submit(Task task) override;
    std::size_t Pending() const noexcept override;
    void Stop();

private:
    struct WorkItem { TaskId id; Task task; };
    void Worker(std::stop_token token, std::size_t index);

    std::string m_name;
    mutable std::mutex m_mutex;
    std::condition_variable_any m_cv;
    std::deque<WorkItem> m_queue;
    std::vector<std::jthread> m_workers;
    std::atomic<TaskId> m_nextTask{0};
    bool m_accepting = true;
};

class ThreadManager
{
public:
    ThreadManager();
    ~ThreadManager();

    void Start();
    void Stop();

    IExecutor& Workers();
    IExecutor& IO();
    IExecutor& CreateDedicated(std::string name);

private:
    std::unique_ptr<QueueExecutor> m_workers;
    std::unique_ptr<QueueExecutor> m_io;
    std::vector<std::unique_ptr<QueueExecutor>> m_dedicated;
};
}
