/* libutil/webserver.h */
#ifndef LIBUTIL_WEBSERVER_H
#define LIBUTIL_WEBSERVER_H 1

#include <string>
#include <list>
#include "stream.h"
#include "poll.h"
#include "worker_thread_pool.h"
#include "task.h"
#include "socket.h"

namespace util {

/** A WebServer plug-in, responsible for all the content under a certain root.
 */
class ContentFactory
{
public:
    virtual ~ContentFactory() {}

    /** Return SeekableStreamPtr() if you don't recognise path.
     */
    virtual SeekableStreamPtr StreamForPath(const char *path) = 0;
};

/** A ContentFactory which exposes a directory on the server's filesystem.
 */
class FileContentFactory: public ContentFactory
{
    std::string m_file_root;
    std::string m_page_root;
public:
    FileContentFactory(const std::string& file_root,
		       const std::string& page_root)
	: m_file_root(file_root), m_page_root(page_root) {}

    // Being a ContentFactory
    SeekableStreamPtr StreamForPath(const char *path);
};

/** An HTTP/1.1 web server.
 */
class WebServer: public Pollable
{
    typedef std::list<ContentFactory*> list_t;
    list_t m_content;
    SeekableStreamPtr m_cache;
    std::string m_cached_url;
    StreamSocket m_server_socket;
    unsigned short m_port;
    WorkerThreadPool m_threads;

    class Task;
    friend class Task;

public:
    WebServer();
    ~WebServer();

    /** Pass port==0 to get a random unassigned port
     */
    unsigned Init(util::PollerInterface*, const char *file_root, 
		  unsigned short port);

    unsigned short GetPort();

    /** Mounts a virtual filesystem on the given "mount point".
     *
     * Note that cf->StreamForPath should also check against page_root, as
     * it may be handed requests for other pages. This also lets you mount
     * the same ContentFactory at several mount-points.
     */
    void AddContentFactory(const char *page_root, ContentFactory *cf);

    SeekableStreamPtr StreamForPath(const char *path);

    // Being a Pollable
    unsigned OnActivity();
};

} // namespace util

#endif
