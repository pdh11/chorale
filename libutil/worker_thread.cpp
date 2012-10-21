#include "trace.h"
#include "worker_thread.h"
#include "task.h"
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include <iostream>
#include <sys/types.h>
#include <linux/unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/resource.h>

namespace util {

class WorkerThread::Impl
{
    TaskQueue *m_queue;
    Priority m_prio;
    
public:
    Impl(TaskQueue *q, Priority p) 
	: m_queue(q), m_prio(p) {}

    pthread_t m_thread;

    void Run();
};

static void *WorkerThreadRun(void *p)
{
    WorkerThread::Impl *wti = (WorkerThread::Impl*)p;

    wti->Run();
    return NULL;
}

WorkerThread::WorkerThread(TaskQueue *queue, Priority p)
    : m_impl(new Impl(queue, p))
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setschedpolicy(&attr, SCHED_OTHER);

    int rc = pthread_create(&m_impl->m_thread, &attr,
			    WorkerThreadRun, m_impl);

    if (rc != 0)
    {
	TRACE << "Couldn't start thread (" << rc << ")\n";
    }
}

WorkerThread::~WorkerThread()
{
//    TRACE << "~Worker joins\n";
    void *result;
    pthread_join(m_impl->m_thread, &result);
    delete m_impl;
//    TRACE << "~Worker done\n";
}

#if 0
#ifndef __NR_gettid
#define __NR_gettid		186
#endif

pid_t gettid()
{
    return syscall(__NR_gettid);
}
#endif

void WorkerThread::Impl::Run()
{
    if (m_prio == LOW)
    {
	/* Both in linuxthreads and in NPTL, this affects only this thread, not
	 * the whole process (fortunately).
	 */
	setpriority(PRIO_PROCESS, 0, 15);

//	TRACE << "getpriority=" << getpriority(PRIO_PROCESS, tid) << "\n";
//	TRACE << "getpriority(0)=" << getpriority(PRIO_PROCESS, 0) << "\n";
//	TRACE << "getpriority(getpid())=" << getpriority(PRIO_PROCESS, getpid()) << "\n";
    }

    for (;;)
    {
	TaskPtr p = m_queue->PopTask();
	if (!p)
	{
	    return;
	}

	p->Run();
    }
}

#if 0
double doo(double);

volatile double vd;

class WaitTask: public Task
{
    std::string m_s;
public:
    WaitTask(const std::string& s) : m_s(s) {}

    void Run()
    {
	std::cout << m_s;
	double d = 0;
	for (unsigned int j = 0; j<2; ++j)
	    for (unsigned int i = 0; i<1000000; ++i)
		d += doo(i/100.0*sin(j*0.2));

	vd = d;
    }
};

void TestThreads()
{
    TaskQueue tq1;
    WorkerThread wt1(&tq1, WorkerThread::NORMAL);
    WorkerThread wt3(&tq1, WorkerThread::NORMAL);
    TaskQueue tq2;
    WorkerThread wt2(&tq2, WorkerThread::LOW);
    WorkerThread wt4(&tq2, WorkerThread::LOW);

    TRACE << "Pushing\n";
    for (unsigned int i=0; i<10; ++i)
    {
	tq1.PushTask(TaskPtr(new WaitTask("normal\n")));
	tq2.PushTask(TaskPtr(new WaitTask("low\n")));
    }
    for (;;)
	sleep(100);
}

double doo(double dee)
{
    return sin(dee) * 2.4 / cos(dee);
}
#endif

}; // namespace util
