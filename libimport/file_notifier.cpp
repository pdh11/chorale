#include "config.h"
#include "file_notifier.h"
#ifdef HAVE_INOTIFY_INIT
#include <sys/inotify.h>
#define HAVE_NOTIFY 1
#endif
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include "libutil/trace.h"
#include "libutil/file.h"
#include "libutil/poll.h"
#undef IN
#undef OUT

#ifndef HAVE_INOTIFY_INIT
# ifdef HAVE_NR_INOTIFY_INIT
#  ifdef HAVE_LINUX_UNISTD_H
#   include <linux/unistd.h>
#  endif
#  ifdef HAVE_LINUX_INOTIFY_H
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

namespace import {

FileNotifier::FileNotifier()
    : m_fd(-1)
{
}

unsigned int FileNotifier::Init(util::PollerInterface *poller)
{
#ifdef HAVE_NOTIFY
    m_fd = inotify_init();
    if (m_fd < 0)
    {
	TRACE << "Can't initialise inotify (" << errno << ")\n";
	return errno;
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

    poller->AddHandle(m_fd, this, util::PollerInterface::IN);
    return 0;
#else
    return ENOSYS;
#endif
}

FileNotifier::~FileNotifier()
{
    if (m_fd != -1)
	close(m_fd);
}

void FileNotifier::Watch(const char *directory)
{
#ifdef HAVE_NOTIFY
    inotify_add_watch(m_fd, directory,
		      IN_ATTRIB | IN_CLOSE_WRITE | IN_CREATE | IN_DELETE |
		      IN_MODIFY | IN_MOVE_SELF | IN_MOVED_FROM | IN_MOVED_TO);
#endif
}

unsigned int FileNotifier::OnActivity()
{
    enum { BUFSIZE = 1024 };
    char buffer[BUFSIZE];
    bool any = false;
    int rc;

    do {
	rc = read(m_fd, buffer, BUFSIZE);
	if (rc > 0)
	    any = true;
    } while (rc > 0);

    if (m_obs)
	m_obs->OnChange();

    return 0;
}

} // namespace import

#ifdef TEST

class TestObserver: public import::FileNotifier::Observer
{
public:
    int n;

    TestObserver() : n(0) {}

    void OnChange()
    {
	TRACE << "changed\n";
	++n;
    }
};

int main()
{
    util::Poller poller;

    import::FileNotifier fn;

    unsigned int rc = fn.Init(&poller);
    if (rc == ENOSYS)
    {
	fprintf(stderr, "No inotify() available -- skipping file_notifier test\n");
	return 0;
    }

    assert(rc == 0);

    char root[] = "file_notifier.test.XXXXXX";

    if (!mkdtemp(root))
    {
	fprintf(stderr, "Can't create temporary dir\n");
	return 1;
    }

    TestObserver obs;
    fn.SetObserver(&obs);
    fn.Watch(root);

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
    system(rmrf.c_str());

    return 0;
}

#endif
