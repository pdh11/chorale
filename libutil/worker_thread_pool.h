#ifndef WORKER_THREAD_POOL_H
#define WORKER_THREAD_POOL_H 1

#include "task.h"
#include <vector>

namespace util {

class WorkerThread;

class WorkerThreadPool
{
    TaskQueue m_queue;
    std::vector<WorkerThread*> m_threads;

public:
    /** Create a thread pool with n threads. 
     *
     * If n=0, use one per CPU.
     */
    explicit WorkerThreadPool(unsigned int n=0);

    ~WorkerThreadPool();

    TaskQueue *GetTaskQueue() { return &m_queue; }
};

}; // namespace util

#endif
