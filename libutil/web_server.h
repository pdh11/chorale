#ifndef LIBUTIL_WEB_SERVER_H
#define LIBUTIL_WEB_SERVER_H 1

#include <string>
#include <list>
#include "stream.h"
#include "poll.h"
#include "worker_thread_pool.h"
#include "task.h"
#include "socket.h"

namespace util {

/** Passed to the ContentFactory, filled-in by the WebServer.
 */
struct WebRequest
{
    std::string path;
    bool refresh;
    IPEndPoint local_ep; // The IP/port on which the client found us

    WebRequest() : refresh(false) {}
};

/** Filled-in by the ContentFactory, passed back to the WebServer.
 */
struct WebResponse
{
    SeekableStreamPtr ssp;
    const char *content_type;   // Default is text/html

    WebResponse() : content_type(NULL) {}
};

/** A WebServer plug-in, responsible for all the content under a certain root.
 */
class ContentFactory
{
public:
    virtual ~ContentFactory() {}

    /** Return true if you recognise path, false if you don't.
     */
    virtual bool StreamForPath(const WebRequest*, WebResponse*) = 0;
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
    bool StreamForPath(const WebRequest*, WebResponse*);
};

/** An HTTP/1.1 web server.
 */
class WebServer: public Pollable
{
    typedef std::list<ContentFactory*> list_t;
    list_t m_content;
    SeekableStreamPtr m_cache;
    std::string m_cached_url;
    const char *m_cached_type;
    StreamSocket m_server_socket;
    unsigned short m_port;
    WorkerThreadPool m_threads;
    WorkerThreadPool m_async_threads;

    class Task;
    friend class Task;

    TaskQueue *GetAsyncQueue();

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
    void AddContentFactory(const std::string& page_root, ContentFactory *cf);

    void StreamForPath(const WebRequest*, WebResponse*);

    // Being a Pollable
    unsigned OnActivity();
};

} // namespace util

#endif
