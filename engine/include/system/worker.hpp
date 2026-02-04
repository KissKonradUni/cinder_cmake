#pragma once

#include <condition_variable>
#include <cstdint>
#include <memory>
#include <thread>
#include <vector>
#include <deque>

namespace hex {

using JobFn = void(*)(void* data);

struct Job {
    JobFn funcPtr;
    void* data;
};

struct SystemWorker {
    std::thread thread;
    std::mutex mutex;
    std::condition_variable condition;
    std::deque<Job> queue;
    bool stop = false;
};

class SystemWorkerPool {
public:
    SystemWorkerPool();
    ~SystemWorkerPool();

    void initialize(uint32_t numWorkers);
    void shutdown();

protected:
    uint32_t                                   m_numWorkers;
    std::vector<std::unique_ptr<SystemWorker>> m_workers;
};

} // namespace hex