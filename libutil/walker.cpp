#include "config.h"
#include "walker.h"
#include "task.h"
#include "file.h"
#include "counted_pointer.h"
#include "counted_object.h"
#include "bind.h"
#include <dirent.h>
#include "trace.h"
#include <errno.h>
#include <stdio.h>

LOG_DECL(WALKER);

namespace util {

struct DirectoryWalkerState: public util::CountedObject<>
{
    util::Mutex mutex;
    unsigned int flags;
    unsigned int count;
    unsigned int error;
    volatile bool stop;
    DirectoryWalker::Observer *observer;
    TaskQueue *queue;

    DirectoryWalkerState(unsigned int f, DirectoryWalker::Observer *o,
			 TaskQueue *q)
	: flags(f), count(0), error(0), stop(false), observer(o), queue(q)
    {
    }
};

typedef util::CountedPointer<DirectoryWalkerState> StatePtr;


        /* DirectoryWalker::Task */


class DirectoryWalker::Task: public util::Task
{
protected:
    StatePtr m_state;
    DirectoryTaskPtr m_parent_task;

public:
    Task(StatePtr state, DirectoryTaskPtr parent_task)
	: m_state(state), m_parent_task(parent_task)
    {
	util::Mutex::Lock lock(m_state->mutex);
	++m_state->count;
	LOG(WALKER) << "state->count++ now " << m_state->count << "\n";
    }

    ~Task()
    {
	util::Mutex::Lock lock(m_state->mutex);
	if (--m_state->count == 0)
	{
	    // We're last
	    m_state->observer->OnFinished(m_state->error);
	}
	else
	{
	    LOG(WALKER) << "state->count-- now " << m_state->count << "\n";
	}
    }
};


        /* DirectoryWalker::FileTask */


class DirectoryWalker::FileTask final: public DirectoryWalker::Task
{
    Observer::dircookie m_cookie;
    unsigned int m_index;
    std::string m_path;
    std::string m_leaf;
    struct stat m_st;

public:
    FileTask(StatePtr state, DirectoryTaskPtr parent_task,
	     Observer::dircookie cookie,
	     unsigned int index,
	     const std::string& path,
	     const std::string& leaf, const struct stat *st)
	: Task(state, parent_task),
	  m_cookie(cookie),
	  m_index(index),
	  m_path(path.c_str()),
	  m_leaf(leaf.c_str()),
	  m_st(*st)
    {
    }

    unsigned int Run() override;
};

unsigned int DirectoryWalker::FileTask::Run()
{
    if (m_state->stop)
	return 0;

    LOG(WALKER) << "File " << m_path << "\n";

    unsigned int error = m_state->observer->OnFile(m_cookie, m_index, m_path, 
						   m_leaf, &m_st);
    if (error)
    {
	util::Mutex::Lock lock(m_state->mutex);
	if (!m_state->error)
	    m_state->error = error;
	LOG(WALKER) << "Error " << error << ", stopping\n";
	m_state->stop = true;
    }
    return error;
}


        /* DirectoryWalker::DirectoryTask */


class DirectoryWalker::DirectoryTask final: public DirectoryWalker::Task
{
    Observer::dircookie m_parent_cookie;
    unsigned int m_parent_index;
    std::string m_path;
    std::string m_leaf;
    struct stat m_st;
    Observer::dircookie m_cookie;

public:
    DirectoryTask(StatePtr state, DirectoryTaskPtr parent_task,
		  Observer::dircookie parent_cookie,
		  unsigned int parent_index,
		  const std::string& path,
		  const std::string& leaf, const struct stat *st)
	: Task(state, parent_task),
	  m_parent_cookie(parent_cookie),
	  m_parent_index(parent_index),
	  m_path(path.c_str()), // force string copy for thread-safety
	  m_leaf(leaf.c_str()),
	  m_st(*st),
	  m_cookie(0)
	{}
    ~DirectoryTask();
    
    unsigned int Run() override;
};

unsigned int DirectoryWalker::DirectoryTask::Run()
{
    if (m_state->stop)
	return 0;

    for (DirectoryTaskPtr dtp = m_parent_task;
	 dtp;
	 dtp = dtp->m_parent_task)
    {
	if (dtp->m_path == m_path)
	{
	    TRACE << "Avoided loop through " << m_path << "\n";
	    return 0;
	}
    }

    LOG(WALKER) << "Entering " << m_path << "\n";

    m_state->observer->OnEnterDirectory(m_parent_cookie, m_parent_index, m_path,
				      m_leaf, &m_st, &m_cookie);

    /* An experiment with pushing the tasks in inode order didn't increase
     * overall speed.
     */

    std::vector<util::Dirent> entries;

    unsigned int rc = ReadDirectory(m_path, &entries);
    if (rc != 0)
	return rc;
    
    /** Each child has a reference to its parent, so that the parent isn't
     * destroyed (and the LeaveDirectory called) until all children are done.
     *
     * The assignment from 'this' only works because we're a
     * boost::intrusive_ptr not a boost::shared_ptr.
     */
    DirectoryTaskPtr me = DirectoryTaskPtr(this);

    unsigned index = 0;

    for (std::vector<util::Dirent>::iterator i = entries.begin();
	 i != entries.end();
	 ++i)
    {
	if (i->name[0] == '.')
	    continue;

	std::string child = m_path + "/" + i->name;

#ifdef S_ISLNK
	// Canonicalise is expensive, only do it when we have a link
	if (S_ISLNK(i->st.st_mode))
	{
	    std::string s = util::Canonicalise(child);

	    if (s.empty())
	    {
		// Dangling link
	    }
	    else
	    {
		child = s;
		::lstat(child.c_str(), &i->st);
	    }
	}
#endif
	
	TaskCallback tc;

	if (S_ISREG(i->st.st_mode))
	{
	    tc = Bind(TaskPtr(new FileTask(m_state, me, m_cookie, index++,
					   child, i->name, &i->st)))
		.To<&Task::Run>();
	    m_state->queue->PushTaskFront(tc);
	}
	else if (S_ISDIR(i->st.st_mode))
	{
	    tc = Bind(TaskPtr(new DirectoryTask(m_state, me, m_cookie, index++,
						child, i->name, &i->st)))
		.To<&Task::Run>();
	    m_state->queue->PushTask(tc);
	}
	else
	{
	    TRACE << "Don't like " << child << "\n";
	}
    }
    return 0;
}

DirectoryWalker::DirectoryTask::~DirectoryTask()
{
    m_state->observer->OnLeaveDirectory(m_cookie, m_path, m_leaf, &m_st);
}


        /* DirectoryWalker */


unsigned int DirectoryWalker::Walk(const std::string& root, Observer *obs,
				   TaskQueue *queue, unsigned int flags)
{
    struct stat st;
    if (::stat(root.c_str(), &st) < 0)
	return errno;

    StatePtr state(new DirectoryWalkerState(flags, obs, queue));

    DirectoryTaskPtr task(new DirectoryTask(state, DirectoryTaskPtr(), 0, 0, 
					    root, 
					    util::GetLeafName(root.c_str()), 
					    &st));
    queue->PushTask(Bind(task).To<&DirectoryTask::Run>());
    return 0;
}

} // namespace util

#ifdef TEST

# include "worker_thread_pool.h"
# include "locking.h"
# include <stdlib.h>

/* Really we're just testing the loop avoidance */

class TestObserver: public util::DirectoryWalker::Observer
{
    size_t m_dircount;
    size_t m_filecount;
    bool m_done;

    util::Mutex m_mutex;
    util::Condition m_finished;

public:
    TestObserver() : m_dircount(0), m_filecount(0), m_done(false) {}

    unsigned int OnEnterDirectory(dircookie parent_cookie,
				  unsigned int index,
				  const std::string& path,
				  const std::string& leaf,
				  const struct stat *st,
				  dircookie *cookie_out);

    unsigned int OnLeaveDirectory(dircookie cookie,
				  const std::string& path,
				  const std::string& leaf,
				  const struct stat*);
    
    unsigned int OnFile(dircookie parent_cookie,
			unsigned int index,
			const std::string& path, 
			const std::string& leaf,
			const struct stat *st);

    void OnFinished(unsigned int error);

    size_t GetDirCount() const { return m_dircount; }
    size_t GetFileCount() const { return m_filecount; }

    void WaitForCompletion();
};

unsigned int TestObserver::OnEnterDirectory(dircookie, unsigned int,
					    const std::string&, 
					    const std::string&,
					    const struct stat*,
					    dircookie *cookie_out)
{
    util::Mutex::Lock lock(m_mutex);

    ++m_dircount;
    if (m_dircount > 100)
	return ERANGE;

    *cookie_out = 0;
    return 0;
}

unsigned int TestObserver::OnLeaveDirectory(dircookie, const std::string&,
					    const std::string&,
					    const struct stat*)
{
    return 0;
}
    
unsigned int TestObserver::OnFile(dircookie, unsigned int, const std::string&, 
				  const std::string&, const struct stat*)
{
    util::Mutex::Lock lock(m_mutex);

    ++m_filecount;
    if (m_filecount > 100)
	return ERANGE;
    return 0;
}

void TestObserver::OnFinished(unsigned int error)
{
    assert(error == 0);

    TRACE << "Done\n";

    util::Mutex::Lock lock(m_mutex);
    m_done = true;
    m_finished.NotifyAll();
}

void TestObserver::WaitForCompletion()
{
    util::Mutex::Lock lock(m_mutex);
    if (m_done)
	return;

    bool did_finish = m_finished.Wait(lock, 60);
    assert(did_finish);
}

int main()
{
    util::WorkerThreadPool wtp(util::WorkerThreadPool::NORMAL);

    char root[] = "file_scanner.test.XXXXXX";

    if (!mkdtemp(root))
    {
	fprintf(stderr, "Can't create temporary dir\n");
	return 1;
    }

    std::string fullname = util::Canonicalise(root);

    mkdir((fullname + "/a").c_str(), 0755);
    mkdir((fullname + "/b").c_str(), 0755);
    FILE *f = fopen((fullname + "/a/x").c_str(), "w+"); fclose(f);
    f = fopen((fullname + "/b/y").c_str(), "w+"); fclose(f);

    util::posix::MakeRelativeLink(fullname + "/a/1", fullname + "/b");
    util::posix::MakeRelativeLink(fullname + "/b/2", fullname + "/a");

    TestObserver obs;
    unsigned int rc = util::DirectoryWalker::Walk(fullname, &obs, &wtp);
    obs.WaitForCompletion();

    assert(obs.GetDirCount() == 5);
    assert(obs.GetFileCount() == 4);

    /* Tidy up */

    std::string rmrf = "rm -r " + fullname;
    rc = system(rmrf.c_str());
    assert(rc == 0);

    wtp.Shutdown();

    return 0;
}

#endif
