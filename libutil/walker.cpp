#include "config.h"
#include "walker.h"
#include "task.h"
#include "file.h"
#include "counted_pointer.h"
#include <dirent.h>
#include "trace.h"
#include <errno.h>
#include <stdio.h>

namespace util {


        /* DirectoryWalker::Task */

class DirectoryWalker::Task: public util::Task
{
protected:
    DirectoryWalker *m_parent;
    DirectoryTaskPtr m_parent_task;

public:
    Task(DirectoryWalker *parent, DirectoryTaskPtr parent_task)
	: m_parent(parent), m_parent_task(parent_task)
    {
	util::Mutex::Lock lock(m_parent->m_mutex);
	++m_parent->m_count;
//	TRACE << m_parent->m_count << "\n";
    }

    ~Task()
    {
	util::Mutex::Lock lock(m_parent->m_mutex);
	if (--m_parent->m_count == 0)
	{
	    // We're last
	    m_parent->m_finished.NotifyOne();
	    m_parent->m_obs->OnFinished(m_parent->m_error);
	}
	else
	{
//	    TRACE << "~" << m_parent->m_count << "\n";
	}
    }
};


        /* DirectoryWalker::FileTask */


class DirectoryWalker::FileTask: public DirectoryWalker::Task
{
    Observer::dircookie m_cookie;
    unsigned int m_index;
    std::string m_path;
    std::string m_leaf;
    struct stat m_st;

public:
    FileTask(DirectoryWalker *parent, DirectoryTaskPtr parent_task,
	     Observer::dircookie cookie,
	     unsigned int index,
	     const std::string& path,
	     const std::string& leaf, const struct stat *st)
	: Task(parent, parent_task),
	  m_cookie(cookie),
	  m_index(index),
	  m_path(path.c_str()),
	  m_leaf(leaf.c_str()),
	  m_st(*st)
    {
    }

    unsigned int Run();
};

unsigned int DirectoryWalker::FileTask::Run()
{
    if (m_parent->m_stop)
	return 0;

    unsigned int error = m_parent->m_obs->OnFile(m_cookie, m_index, m_path, 
						 m_leaf, &m_st);
    if (error)
    {
	util::Mutex::Lock lock(m_parent->m_mutex);
	if (!m_parent->m_error)
	    m_parent->m_error = error;
	m_parent->m_stop = true;
    }
    return error;
}


        /* DirectoryWalker::DirectoryTask */


class DirectoryWalker::DirectoryTask: public DirectoryWalker::Task
{
    Observer::dircookie m_parent_cookie;
    unsigned int m_parent_index;
    std::string m_path;
    std::string m_leaf;
    struct stat m_st;
    Observer::dircookie m_cookie;

public:
    DirectoryTask(DirectoryWalker *parent, DirectoryTaskPtr parent_task,
		  Observer::dircookie parent_cookie,
		  unsigned int parent_index,
		  const std::string& path,
		  const std::string& leaf, const struct stat *st)
	: Task(parent, parent_task),
	  m_parent_cookie(parent_cookie),
	  m_parent_index(parent_index),
	  m_path(path.c_str()), // force string copy for thread-safety
	  m_leaf(leaf.c_str()),
	  m_st(*st),
	  m_cookie(0)
	{}
    ~DirectoryTask();
    
    unsigned int Run();
};

unsigned int DirectoryWalker::DirectoryTask::Run()
{
    if (m_parent->m_stop)
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

    m_parent->m_obs->OnEnterDirectory(m_parent_cookie, m_parent_index, m_path,
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
	
	TaskPtr t;

	if (S_ISREG(i->st.st_mode))
	{
	    t = TaskPtr(new FileTask(m_parent, me, m_cookie, index++,
				     child, i->name, &i->st));
	}
	else if (S_ISDIR(i->st.st_mode))
	{
	    t = TaskPtr(new DirectoryTask(m_parent, me, m_cookie, index++,
					  child, i->name, &i->st));
	}
	else
	{
	    TRACE << "Don't like " << child << "\n";
	}
	if (t)
	    m_parent->m_queue->PushTask(Bind<util::Task,&util::Task::Run>(t));
    }
    return 0;
}

DirectoryWalker::DirectoryTask::~DirectoryTask()
{
    m_parent->m_obs->OnLeaveDirectory(m_cookie, m_path, m_leaf, &m_st);
}


        /* DirectoryWalker */


DirectoryWalker::DirectoryWalker(const std::string& root, Observer *obs,
				 TaskQueue *queue, unsigned int flags)
    : m_root(root),
      m_obs(obs),
      m_queue(queue),
      m_flags(flags),
      m_count(0),
      m_error(0),
      m_stop(false)
{
}

void DirectoryWalker::Start()
{
    struct stat st;
    ::stat(m_root.c_str(), &st);
    m_queue->PushTask(Bind<util::Task,&util::Task::Run>(TaskPtr(new DirectoryTask(this, DirectoryTaskPtr(), 
						0, 0, m_root,
						util::GetLeafName(m_root.c_str()),
								      &st))));
}

void DirectoryWalker::WaitForCompletion()
{
    util::Mutex::Lock lock(m_mutex);

    while (m_count)
    {
	m_finished.Wait(lock, 60);
    }
}

} // namespace util

#ifdef TEST

# include "worker_thread_pool.h"
# include "locking.h"
# include <stdlib.h>

/* Really we're just testing the loop avoidance */

class TestObserver: public util::DirectoryWalker::Observer,
		    public util::PerObjectLocking
{
    size_t m_dircount;
    size_t m_filecount;

    typedef util::PerObjectLocking LockingPolicy;

public:
    TestObserver() : m_dircount(0), m_filecount(0) {}

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
};

unsigned int TestObserver::OnEnterDirectory(dircookie, unsigned int,
					    const std::string&, 
					    const std::string&,
					    const struct stat*,
					    dircookie *cookie_out)
{
    LockingPolicy::Lock(this);

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
    LockingPolicy::Lock(this);

    ++m_filecount;
    if (m_filecount > 100)
	return ERANGE;
    return 0;
}

void TestObserver::OnFinished(unsigned int error)
{
    assert(error == 0);
}

int main()
{
#ifndef WIN32
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
    util::DirectoryWalker dw(fullname, &obs, &wtp);
    dw.Start();
    dw.WaitForCompletion();

    assert(obs.GetDirCount() == 5);
    assert(obs.GetFileCount() == 4);

    /* Tidy up */

    std::string rmrf = "rm -r " + fullname;
    int rc = system(rmrf.c_str());
    assert(rc == 0);

    wtp.Shutdown();

#endif
    return 0;
}

#endif
