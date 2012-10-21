#ifndef UTIL_WALKER_H
#define UTIL_WALKER_H

#include <sys/stat.h>
#include <string>
#include "mutex.h"

namespace util
{

template <class> class CountedPointer;

class TaskQueue;

/** Uses one or more background threads to do the scanning (as determined
 * by the TaskQueue passed in). The observer may be called back on any of
 * those threads, or several simultaneously.
 */
class DirectoryWalker
{
public:
    class Observer;

    /** Values of flags */
    enum {
	REPORT_LINKS   = 0x1, // Otherwise, follow them silently
	ONE_FILESYSTEM = 0x2
    };

private:
    std::string m_root;
    Observer *m_obs;
    TaskQueue *m_queue;
    unsigned int m_flags;
    util::Mutex m_mutex;
    util::Condition m_finished;
    unsigned int m_count;
    unsigned int m_error;
    volatile bool m_stop;

    class Task;
    class FileTask;
    class DirectoryTask;
    class SymbolicLinkTask;

    friend class Task;
    friend class FileTask;
    friend class DirectoryTask;
    friend class SymbolicLinkTask;

    typedef util::CountedPointer<DirectoryTask> DirectoryTaskPtr;

public:

    class Observer
    {
    public:
	virtual ~Observer() {}

	typedef size_t dircookie;

	/** Return an error (nonzero) to abort the scan */
	
	virtual unsigned int OnEnterDirectory(dircookie parent_cookie,
					      unsigned int index,
					      const std::string& path,
					      const std::string& leaf,
					      const struct stat *st,
					      dircookie *cookie_out) = 0;

	virtual unsigned int OnLeaveDirectory(dircookie cookie,
					      const std::string& path,
					      const std::string& leaf,
					      const struct stat *st) = 0;

	virtual unsigned int OnFile(dircookie parent_cookie,
				    unsigned int index,
				    const std::string& path, 
				    const std::string& leaf,
				    const struct stat *st) = 0;
    
/*	virtual unsigned int OnSymbolicLink(dircookie parent_cookie,
					    unsigned int index,
					    const std::string& path,
					    const std::string& leaf,
					    const struct stat *st) = 0; */

	/** Called once everything's finished. */
	virtual void OnFinished(unsigned int error) = 0;
    };

    DirectoryWalker(const std::string& root, Observer *obs,
		    TaskQueue *queue, unsigned int flags=0);

    void Start();
    void Cancel();
    void WaitForCompletion();
};

} // namespace util

#endif
