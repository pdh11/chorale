#include "trace.h"
#include "worker_thread.h"
#include "task.h"
#include "config.h"
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <unistd.h>
#include <math.h>
#include <iostream>
#include <sys/types.h>
#include <errno.h>
#include <sys/time.h>
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

namespace util {

class WorkerThread::Impl
{
    TaskQueue *m_queue;
    Priority m_prio;
    
public:
    Impl(TaskQueue *q, Priority p) 
	: m_queue(q),
	  m_prio(p),
	  m_thread(boost::bind(&Impl::Run, this))
	{}
    ~Impl()
    {
	m_thread.join();
    }

    boost::thread m_thread;

    void Run();
};

WorkerThread::WorkerThread(TaskQueue *queue, Priority p)
    : m_impl(new Impl(queue, p))
{
}

WorkerThread::~WorkerThread()
{
//    TRACE << "~Worker joins\n";
    delete m_impl;
//    TRACE << "~Worker done\n";
}

void WorkerThread::Impl::Run()
{
    if (m_prio == LOW)
    {
#ifdef WIN32
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);
#elif defined(HAVE_SETPRIORITY)
	/* Both in linuxthreads and in NPTL, this affects only this thread, not
	 * the whole process (fortunately).
	 */
	setpriority(PRIO_PROCESS, 0, 15);
#endif
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

} // namespace util
