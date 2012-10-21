#ifndef WORKER_THREAD_H
#define WORKER_THREAD_H

namespace util {

class TaskQueue;

class WorkerThread
{
public:
    class Impl;
private:
    Impl *m_impl;

public:
    
    enum Priority {
	NORMAL,
	LOW
    };

    WorkerThread(TaskQueue *queue, Priority p = NORMAL);
    ~WorkerThread();
};

void TestThreads();

}; // namespace util

#endif
