#include "config.h"
#include "walker.h"
#include "task.h"
#include "file.h"
#include <dirent.h>
#include "trace.h"
#include <errno.h>

namespace util {


        /* DirectoryWalker::Task */


class DirectoryWalker::Task: public util::Task
{
protected:
    DirectoryWalker *m_parent;
    TaskPtr m_parent_task;

public:
    explicit Task(DirectoryWalker *parent, TaskPtr parent_task)
	: m_parent(parent), m_parent_task(parent_task)
    {
	boost::mutex::scoped_lock lock(m_parent->m_mutex);
	++m_parent->m_count;
//	TRACE << m_parent->m_count << "\n";
    }

    ~Task()
    {
	boost::mutex::scoped_lock lock(m_parent->m_mutex);
	if (--m_parent->m_count == 0)
	{
	    // We're last
	    m_parent->m_finished.notify_one();
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
    FileTask(DirectoryWalker *parent, TaskPtr parent_task,
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

    void Run();
};

void DirectoryWalker::FileTask::Run()
{
    if (m_parent->m_stop)
	return;

    unsigned int error = m_parent->m_obs->OnFile(m_cookie, m_index, m_path, 
						 m_leaf, &m_st);
    if (error)
    {
	boost::mutex::scoped_lock lock(m_parent->m_mutex);
	if (!m_parent->m_error)
	    m_parent->m_error = error;
	m_parent->m_stop = true;
    }
}


        /* DirectoryWalker::SymbolicLinkTask */


#if 0
class DirectoryWalker::SymbolicLinkTask: public DirectoryWalker::Task
{
    Observer::dircookie m_cookie;
    unsigned int m_index;
    std::string m_path;
    std::string m_leaf;
    struct stat m_st;

public:
    SymbolicLinkTask(DirectoryWalker *parent, TaskPtr parent_task,
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

    void Run();
};

void DirectoryWalker::SymbolicLinkTask::Run()
{
    if (m_parent->m_stop)
	return;

    unsigned int error = m_parent->m_obs->OnSymbolicLink(m_cookie, m_index, m_path, 
							 m_leaf, &m_st);
    if (error)
    {
	boost::mutex::scoped_lock lock(m_parent->m_mutex);
	if (!m_parent->m_error)
	    m_parent->m_error = error;
	m_parent->m_stop = true;
    }
}
#endif


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
    DirectoryTask(DirectoryWalker *parent, TaskPtr parent_task,
		  Observer::dircookie parent_cookie,
		  unsigned int parent_index,
		  const std::string& path,
		  const std::string& leaf, const struct stat *st)
	: Task(parent, parent_task),
	  m_parent_cookie(parent_cookie),
	  m_parent_index(parent_index),
	  m_path(path.c_str()), // force string copy for thread-safety
	  m_leaf(leaf.c_str()),
	  m_st(*st)
	{}
    ~DirectoryTask();
    
    void Run();
};

static int walker_scandir_selector_all(const struct dirent*)
{
    return 1;
}

void DirectoryWalker::DirectoryTask::Run()
{
    if (m_parent->m_stop)
	return;

    m_parent->m_obs->OnEnterDirectory(m_parent_cookie, m_parent_index, m_path,
				      m_leaf, &m_st, &m_cookie);

    /* An experiment with pushing the tasks in inode order didn't increase
     * overall speed.
     */

    struct dirent **namelist;
    int rc = scandir(m_path.c_str(), &namelist, walker_scandir_selector_all,
		     versionsort);

    if (rc <= 0)
    {
	TRACE << "scandir doesn't like " << m_path << ": " << rc
	      << " errno=" << errno << "\n";
	return;
    }
    
    /** Each child has a reference to its parent, so that the parent isn't
     * destroyed (and the LeaveDirectory called) until all children are done.
     *
     * The assignment from 'this' only works because we're a
     * boost::intrusive_ptr not a boost::shared_ptr.
     */
    TaskPtr me = TaskPtr(this);

    unsigned index = 0;

    for (unsigned int i=0; i<(unsigned int)rc; ++i)
    {
	struct dirent *de = namelist[i];

	if (de->d_name[0] == '.')
	{
	    free(de);
	    continue;
	}

	std::string child = m_path + "/";
	child += de->d_name;

	// Canonicalise is expensive, only do it when we have a link
	struct stat st;
	::lstat(child.c_str(), &st);
	if (S_ISLNK(st.st_mode))
	{
	    std::string s = util::Canonicalise(child);

	    if (s.empty())
	    {
		// Dangling link
	    }
	    else
	    {
		child = s;
		::lstat(child.c_str(), &st);
	    }
	}

	/** @todo Deny links to parent directories of the link or of the root
	 */
	if (de->d_type == DT_UNKNOWN || de->d_type == DT_LNK)
	{
	    if (S_ISREG(st.st_mode))
		de->d_type = DT_REG;
	    else if (S_ISDIR(st.st_mode))
		de->d_type = DT_DIR;
	    else if (S_ISLNK(st.st_mode))
		de->d_type = DT_LNK;
	}
	
	TaskPtr t;

	if (de->d_type == DT_REG)
	{
	    t = TaskPtr(new FileTask(m_parent, me, m_cookie, index++,
				     child, de->d_name, &st));
	}
	else if (de->d_type == DT_DIR)
	{
	    t = TaskPtr(new DirectoryTask(m_parent, me, m_cookie, index++,
					  child, de->d_name, &st));
	}
	else if (de->d_type == DT_LNK)
	{
//	    t = TaskPtr(new SymbolicLinkTask(m_parent, me, m_cookie, index++,
//					     child, de->d_name, &st));
	}
	else
	{
	    TRACE << "Don't like " << child << " (" << de->d_type << ")\n";
	}

	free(de);

	if (t)
	    m_parent->m_queue->PushTask(t);
    }
    free(namelist);
}

DirectoryWalker::DirectoryTask::~DirectoryTask()
{
    m_parent->m_obs->OnLeaveDirectory(m_cookie, m_path, m_leaf);
}


        /* DirectoryWalker */


DirectoryWalker::DirectoryWalker(const std::string& root, Observer *obs,
				 TaskQueue *queue)
    : m_root(root),
      m_obs(obs),
      m_queue(queue),
      m_count(0),
      m_error(0),
      m_stop(false)
{
}

void DirectoryWalker::Start()
{
    struct stat st;
    ::stat(m_root.c_str(), &st);
    m_queue->PushTask(TaskPtr(new DirectoryTask(this, TaskPtr(), 0, 0, m_root,
						util::GetLeafName(m_root.c_str()),
						&st)));
}

void DirectoryWalker::WaitForCompletion()
{
    boost::mutex::scoped_lock lock(m_mutex);

    while (m_count)
    {
	m_finished.wait(lock);
    }
}

} // namespace util
