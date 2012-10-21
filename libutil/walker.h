#ifndef UTIL_WALKER_H
#define UTIL_WALKER_H

#include <sys/stat.h>
#include <string>

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

    /** Values of flags */
    enum {
	REPORT_LINKS   = 0x1, // Otherwise, follow them silently
	ONE_FILESYSTEM = 0x2
    };

    class Task;
    class FileTask;
    class DirectoryTask;

    typedef util::CountedPointer<DirectoryTask> DirectoryTaskPtr;

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

	/** Called once everything's finished. */
	virtual void OnFinished(unsigned int error) = 0;
    };

    static unsigned int Walk(const std::string& root, Observer *obs,
			     TaskQueue *queue, unsigned int flags=0);
};

} // namespace util

#endif
