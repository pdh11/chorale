#include "page_locking.h"
#include "file.h"
#include "libutil/trace.h"
#include "libutil/errors.h"

LOG_DECL(LOCKING);

namespace isam {

size_t s_max_waiting_readers = 0;
size_t s_max_waiting_writers = 0;

PageLocking::PageLocking(File *file)
    : m_file(file)
{
}

PageLocking::~PageLocking()
{
    TRACE << "Max waiting writers " << s_max_waiting_writers
	  << " readers " << s_max_waiting_readers << "\n";
}

const void *PageLocking::LockForRead(unsigned int pageno)
{
    util::Mutex::Lock lock(m_mutex);
    bool inserted = false;
    
    while (m_write_locked.find(pageno) != m_write_locked.end()
	   || m_write_waiting.find(pageno) != m_write_waiting.end())
    {
	if (!inserted)
	{
	    m_read_waiting.insert(pageno);

	    size_t count = m_read_waiting.size();
	    if (count > s_max_waiting_readers)
		s_max_waiting_readers = count;
	    inserted = true;
	}

	LOG(LOCKING) << "Waiting for read lock on " << pageno << "\n";
	m_readable.Wait(lock, 60);
	LOG(LOCKING) << "Waitread done\n";
    }

//    TRACE << "Locked " << pageno << " for read\n";
	
    // NOT multiset.erase(pageno) as that erases ALL matches
    if (inserted)
	m_read_waiting.erase(m_read_waiting.find(pageno));
    m_read_locked.insert(pageno);

    return (const void*)m_file->Page(pageno);
}

void PageLocking::UnlockForRead(unsigned int pageno)
{
    util::Mutex::Lock lock(m_mutex);
    
    // NOT multiset.erase(pageno) as that erases ALL matches
    m_read_locked.erase(m_read_locked.find(pageno));

//    TRACE << "Unlocked " << pageno << " for read\n";

    if (m_write_waiting.find(pageno) != m_write_waiting.end())
    {
	LOG(LOCKING) << "Unlocked contended " << pageno << " for read\n";
	m_writable.NotifyAll();
    }
}

void *PageLocking::LockForWrite(unsigned int pageno)
{
    util::Mutex::Lock lock(m_mutex);
    bool inserted = false;

    while (m_read_locked.find(pageno) != m_read_locked.end()
	   || m_write_locked.find(pageno) != m_write_locked.end())
    {
	if (!inserted)
	{
	    m_write_waiting.insert(pageno);

	    size_t count = m_write_waiting.size();
	    if (count > s_max_waiting_writers)
		s_max_waiting_writers = count;
	    inserted = true;
	}

	LOG(LOCKING) << "Waiting for write lock on " << pageno << "\n";
	m_writable.Wait(lock, 60);
	LOG(LOCKING) << "Waitwrite done\n";
    }

//    TRACE << "Locked " << pageno << " for write\n";

    // NOT multiset.erase(pageno) as that erases ALL matches
    if (inserted)
	m_write_waiting.erase(m_write_waiting.find(pageno));
    m_write_locked.insert(pageno);

    return (void*)m_file->Page(pageno);
}

void PageLocking::UnlockForWrite(unsigned int pageno)
{
    {
	util::Mutex::Lock lock(m_mutex);

	m_write_locked.erase(pageno);

//    TRACE << "Unlocked " << pageno << " for write\n";

	if (m_write_waiting.find(pageno) != m_write_waiting.end())
	{
	    LOG(LOCKING) << "Unlocked contended " << pageno
			 << " for write, writer waiting\n";
	    m_writable.NotifyAll();
	}
	else if (m_read_waiting.find(pageno) != m_read_waiting.end())
	{
	    LOG(LOCKING) << "Unlocked contended " << pageno
			 << " for write, reader waiting\n";
	    m_readable.NotifyAll();
	}
    }
    m_file->WriteOut(pageno);
}


        /* ReadLockChain */


ReadLockChain::ReadLockChain(PageLocking *locking)
    : m_locking(locking),
      m_page((unsigned int)-1)
{
}

ReadLockChain::~ReadLockChain()
{
    if (m_page != (unsigned int)-1)
	m_locking->UnlockForRead(m_page);
}

unsigned int ReadLockChain::Add(unsigned int pageno)
{
    const void *ptr = m_locking->LockForRead(pageno);
    if (!ptr)
	return EINVAL;

    if (m_page != (unsigned int)-1)
	m_locking->UnlockForRead(m_page);
    m_page = pageno;
    return 0;
}


        /* WriteLockChain */


WriteLockChain::WriteLockChain(PageLocking *locking)
    : m_locking(locking),
      m_parent_page((unsigned int)-1),
      m_child_page((unsigned int)-1)
{
}

WriteLockChain::~WriteLockChain()
{
    if (m_child_page != (unsigned int)-1)
	m_locking->UnlockForWrite(m_child_page);
    if (m_parent_page != (unsigned int)-1)
	m_locking->UnlockForWrite(m_parent_page);
}

unsigned int WriteLockChain::Add(unsigned int pageno)
{
    void *ptr = m_locking->LockForWrite(pageno);
    if (!ptr)
	return EINVAL;

    if (m_parent_page != (unsigned int)-1)
	m_locking->UnlockForWrite(m_parent_page);
    m_parent_page = m_child_page;
    m_child_page = pageno;
    return 0;
}


        /* DeleteLockChain */


DeleteLockChain::DeleteLockChain(PageLocking *locking)
    : m_locking(locking)
{
}

DeleteLockChain::~DeleteLockChain()
{
    while (!m_pages.empty())
    {
	unsigned int page = m_pages.back();
	m_pages.pop_back();
	m_locking->UnlockForWrite(page);
    }
}

unsigned int DeleteLockChain::Add(unsigned int pageno,
				  unsigned int siblings)
{
    void *ptr = m_locking->LockForWrite(pageno);
    if (!ptr)
	return EINVAL;

    m_pages.push_back(pageno);

//    TRACE << "DLC adds page " << pageno << " count=" << siblings << "\n";

    if (siblings > 1)
    {
	while (m_pages.size() > 3)
	    ReleaseParentPage();
    }

//    TRACE << "DLC now " << m_pages;

    return 0;
}

void DeleteLockChain::ReleaseChildPage()
{
    unsigned int page = m_pages.back();
    m_pages.pop_back();
    m_locking->UnlockForWrite(page);
}

void DeleteLockChain::ReleaseParentPage()
{
    unsigned int page = m_pages.front();
    m_pages.pop_front();
    m_locking->UnlockForWrite(page);
//    TRACE << "Released parent, DLC now " << m_pages << "\n";
}

} // namespace isam

#ifdef TEST

# include <stdlib.h>
# include "libutil/worker_thread_pool.h"
# include "libutil/counted_pointer.h"

enum { PAGES = 2100,
	NUM_THREADS = 50,
	NUM_TASKS = 50,
	NUM_REQUESTS = 1000
};

util::Mutex s_mutex;
util::Condition s_finished;
unsigned int s_nfinished = 0;

volatile unsigned int s_result;

class LockTestTask: public util::Task
{
    isam::PageLocking *m_locking;

    void TestReadLock(unsigned int page);
    void TestReadLockChain(unsigned int page);
    void TestWriteLock(unsigned int page);
    void TestWriteLockChain(unsigned int page);

public:
    LockTestTask(isam::PageLocking *locking)
	: m_locking(locking)
	{
	}

    unsigned int Run();
};

unsigned int LockTestTask::Run()
{
    const time_t start = time(NULL);

    do {
	unsigned int which = rand() & 15;
	switch (which)
	{
	case 0:
	    TestWriteLock(rand() % (PAGES/4));
	    break;
	case 1:
	    TestWriteLockChain(rand() % (PAGES/4));
	    break;
	default:
	    if (which & 1)
		TestReadLock(rand() % (PAGES/4));
	    else
		TestReadLockChain(rand() % (PAGES/4));
	    break;
	}
    } while ((time(NULL) - start) < 20);

    util::Mutex::Lock lock(s_mutex);
    ++s_nfinished;
    s_finished.NotifyOne();
    return 0;
}

void LockTestTask::TestReadLock(unsigned int page)
{
    isam::ReadLock lock(m_locking, page);
    const void *ptr = lock.Lock();

    s_result += *(const unsigned int*)ptr;

    /** To avoid deadlocks, individual pages MUST be locked in
     * top-down hierarchical order. We model the hierarchy here by
     * using a heap, where page N's children are pages 2N+1 and 2N+2.
     */

    unsigned int next_page = page*2 + (rand() & 1) + 1;
    if (next_page < PAGES)
	TestReadLock(next_page);
    else
	usleep(100);
}

void LockTestTask::TestWriteLock(unsigned int page)
{
    isam::WriteLock lock(m_locking, page);
    void *ptr = lock.Lock();

    *(unsigned int*)ptr = s_result;
    
    unsigned int next_page = page*2 + (rand() & 1) + 1;
    if (next_page < PAGES)
	TestWriteLock(next_page);
    else
	usleep(100);
}

void LockTestTask::TestReadLockChain(unsigned int page)
{
    isam::ReadLockChain rlc(m_locking);

    for (;;)
    {
	unsigned int rc = rlc.Add(page,2);
	assert(rc == 0);
	
	page = (page*2) + (rand() & 1) + 1;
	if (page >= PAGES)
	{
	    usleep(100);
	    return;
	}
    }
}

void LockTestTask::TestWriteLockChain(unsigned int page)
{
    isam::WriteLockChain wlc(m_locking);

    for (;;)
    {
	unsigned int rc = wlc.Add(page,2);
	assert(rc == 0);
	
	page = (page*2) + (rand() & 1) + 1;
	if (page >= PAGES)
	{
	    usleep(100);
	    return;
	}
    }
}

int main()
{
    isam::File file;

    unsigned int rc = file.Open(NULL, 0);
    assert(rc == 0);

    {
	isam::PageLocking pl(&file);

	{
	    util::WorkerThreadPool wtp(util::WorkerThreadPool::NORMAL,
				       NUM_THREADS);

	    for (unsigned int i=0; i<NUM_TASKS; ++i)
	    {
		util::TaskPtr tp(new LockTestTask(&pl));
		wtp.PushTask(tp);
	    }
	    
	    util::Mutex::Lock lock(s_mutex);
	    while (s_nfinished < NUM_TASKS)
	    {
		s_finished.Wait(lock, 60);
//		TRACE << s_nfinished << "/" << NUM_TASKS << " finished\n";
	    }

//	    TRACE << "Closing wtp\n";
	}

//	TRACE << "Closing locking\n";
    }
	    
//    TRACE << "Closing\n";
    file.Close();

//    TRACE << "Shutting down\n";
    return 0;
}

#endif
