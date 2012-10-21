#include "file_notifier.h"
#include <sys/inotify.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include "libutil/trace.h"
#include "libutil/file.h"

namespace import {

FileNotifier::FileNotifier()
    : m_fd(-1)
{
}

unsigned int FileNotifier::Init(util::PollerInterface *poller)
{
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
}

FileNotifier::~FileNotifier()
{
    if (m_fd != -1)
	close(m_fd);
}

void FileNotifier::Watch(const char *directory)
{
    inotify_add_watch(m_fd, directory,
		      IN_ATTRIB | IN_CLOSE_WRITE | IN_CREATE | IN_DELETE |
		      IN_MODIFY | IN_MOVE_SELF | IN_MOVED_FROM | IN_MOVED_TO);
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

}; // namespace import

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
    char root[] = "file_notifier.test.XXXXXX";

    if (!mkdtemp(root))
    {
	fprintf(stderr, "Can't create temporary dir\n");
	return 1;
    }

    util::Poller poller;

    import::FileNotifier fn;

    unsigned int rc = fn.Init(&poller);
    assert(rc == 0);

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
