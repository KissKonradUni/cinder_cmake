#include "system/worker.hpp"

namespace hex {

void system_worker_main(SystemWorker* worker) {
    while (true) {
        Job job;

        {
            std::unique_lock lock(worker->mutex);
            worker->condition.wait(lock, [&] {
                return worker->stop || !worker->queue.empty();
            });

            if (worker->stop && worker->queue.empty())
                return;

            job = worker->queue.front();
            worker->queue.pop_front();
        }

        job.funcPtr(job.data);
    }
}

SystemWorkerPool::SystemWorkerPool()
    : m_numWorkers(0) {}

SystemWorkerPool::~SystemWorkerPool() {
    shutdown();
}

void SystemWorkerPool::initialize(uint32_t numWorkers) {
    m_numWorkers = numWorkers;
    m_workers.resize(m_numWorkers);

    for (uint32_t i = 0; i < m_numWorkers; ++i) {
        m_workers[i] = std::make_unique<SystemWorker>();
        m_workers[i]->thread = std::thread(system_worker_main, m_workers[i].get());
    }
}

void SystemWorkerPool::shutdown() {
    for (auto& worker : m_workers) {
        {
            std::unique_lock lock(worker->mutex);
            worker->stop = true;
        }
        worker->condition.notify_all();
        if (worker->thread.joinable()) {
            worker->thread.join();
        }
    }
    m_workers.clear();
    m_numWorkers = 0;
}

} // namespace hex