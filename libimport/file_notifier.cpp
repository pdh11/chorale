#include "config.h"
#include "file_notifier.h"
#include "libutil/bind.h"
#if HAVE_INOTIFY_INIT
#include <sys/inotify.h>
#define HAVE_NOTIFY 1
#endif
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include "libutil/trace.h"
#include "libutil/file.h"
#include "libutil/scheduler.h"
#undef IN
#undef OUT

#if !HAVE_INOTIFY_INIT
# if HAVE_NR_INOTIFY_INIT
#  if HAVE_LINUX_UNISTD_H
#   include <linux/unistd.h>
#  endif
#  if HAVE_LINUX_INOTIFY_H
#   include <linux/inotify.h>
#  endif

static int inotify_init()
{
    return ::syscall(__NR_inotify_init);
}

static int inotify_add_watch(int fd, const char *name, uint32_t mask)
{
    return ::syscall(__NR_inotify_add_watch, fd, name, mask);
}

//static int inotify_rm_watch(int fd, uint32_t wd)
//{
//    return ::syscall(__NR_inotify_rm_watch, fd, wd);
//}

#  define HAVE_NOTIFY 1
# endif
#endif

#ifndef HAVE_NOTIFY
#define HAVE_NOTIFY 0
#endif

namespace import {

FileNotifierPtr FileNotifierTask::Create(util::Scheduler *scheduler)
{
    return FileNotifierPtr(new FileNotifierTask(scheduler));
}

FileNotifierTask::FileNotifierTask(util::Scheduler *scheduler)
    : m_scheduler(scheduler),
      m_obs(NULL),
      m_fd(util::NOT_POLLABLE)
{
}

unsigned int FileNotifierTask::Init()
{
#if HAVE_NOTIFY
    m_fd = inotify_init();
    if (m_fd < 0)
    {
	TRACE << "Can't initialise inotify (" << errno << ")\n";
	return (unsigned)errno;
    }
    else
    {
	int flags = fcntl(m_fd, F_GETFL);
	if (flags >= 0)
	{
	    flags |= O_NONBLOCK;
	    fcntl(m_fd, F_SETFL, flags);
	}
    }
    
    m_scheduler->WaitForReadable(
	util::Bind(FileNotifierPtr(this)).To<&FileNotifierTask::Run>(),
	this, false);

//    TRACE << "Notifier started successfully\n";

    return 0;
#else
    return ENOSYS;
#endif
}

FileNotifierTask::~FileNotifierTask()
{
#if HAVE_NOTIFY
    if (m_fd != -1)
	close(m_fd);
#endif
}

void FileNotifierTask::Watch(const char *directory)
{
#if HAVE_NOTIFY
    inotify_add_watch(m_fd, directory,
		      IN_ATTRIB | IN_CLOSE_WRITE | IN_CREATE | IN_DELETE |
		      IN_MOVE_SELF | IN_MOVED_FROM | IN_MOVED_TO);
#endif
}

unsigned int FileNotifierTask::Run()
{
#if HAVE_NOTIFY
    enum { BUFSIZE = 1024 };
    char buffer[BUFSIZE];
    bool any = false;
    ssize_t rc;

    do {
	rc = read(m_fd, buffer, BUFSIZE);
	if (rc > 0)
	    any = true;
    } while (rc > 0);

    if (m_obs)
	m_obs->OnChange();
#endif

    return 0;
}

} // namespace import

#ifdef TEST

class TestObserver: public import::FileNotifierTask::Observer
{
public:
    int n;

    TestObserver() : n(0) {}

    void OnChange()
    {
//	TRACE << "changed\n";
	++n;
    }
};

int main()
{
    util::BackgroundScheduler poller;

    import::FileNotifierPtr fn(import::FileNotifierTask::Create(&poller));

    unsigned int rc = fn->Init();
    if (rc == ENOSYS)
    {
	fprintf(stderr, "No inotify() available -- skipping file_notifier test\n");
	return 0;
    }

    assert(rc == 0);

#ifndef WIN32
    char root[] = "file_notifier.test.XXXXXX";

    if (!mkdtemp(root))
    {
	fprintf(stderr, "Can't create temporary dir\n");
	return 1;
    }

    TestObserver obs;
    fn->SetObserver(&obs);
    fn->Watch(root);

    poller.Poll(250);

    assert(obs.n == 0);

    std::string child(root);
    child += "/foo.mp3";
    FILE *f = fopen(child.c_str(), "w+");
    fclose(f);

    poller.Poll(500);

    assert(obs.n > 0);

    /* Tidy up */

    std::string rmrf = "rm -r " + util::Canonicalise(root);
    if (system(rmrf.c_str()) < 0)
    {
	fprintf(stderr, "Clean up failed\n");
	return 1;
    }
#endif
    return 0;
}

#endif
