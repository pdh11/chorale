#ifndef LIBUTIL_HTTP_SERVER_H
#define LIBUTIL_HTTP_SERVER_H 1

#include <string>
#include <list>
#include <map>
#include "socket.h"
#include "stream.h"
#include "worker_thread_pool.h"
#include "pollable_task.h"
#include "buffer_chain.h"

namespace util {

class TaskQueue;
class PollerInterface;

namespace http {

/** Caseless string compare, for HTTP headers.
 *
 * Assumed ASCII-only.
 */
struct CaselessCompare
{
    bool operator()(const std::string& s1, const std::string& s2) const
    {
	return strcasecmp(s1.c_str(), s2.c_str()) < 0;
    }
};

/** Passed to the ContentFactory, filled-in by the http::Server.
 */
struct Request: public boost::noncopyable
{
    std::string verb;
    std::string path;
    bool refresh;
    bool has_body;
    IPEndPoint local_ep; /// The IP/port on which the client found us
    typedef std::map<std::string, std::string, CaselessCompare> headers_t;
    headers_t headers;

    void Clear()
    {
	refresh = has_body = false;
	verb.clear();
	path.clear();
	headers.clear();
    }

    std::string GetHeader(const char *s) const
    {
	headers_t::const_iterator i = headers.find(s);
	if (i == headers.end())
	    return std::string();
	return i->second;
    }
};

/** Filled-in by the ContentFactory, passed back to the http::Server.
 */
struct Response: public boost::noncopyable
{
    std::auto_ptr<BufferSink> body_sink;
    SeekableStreamPtr ssp;
    const char *content_type;   /// Default is text/html
    std::map<std::string, std::string> headers;

    void Clear()
    {
	body_sink.reset(NULL);
	content_type = NULL;
	ssp.reset(NULL);
    }
};

/** A http::Server plug-in, responsible for all the content under a
 * certain root.
 *
 * @todo Split out into separate header file to reduce coupling.
 */
class ContentFactory
{
public:
    virtual ~ContentFactory() {}

    /** Return true if you recognise path, false if you don't.
     */
    virtual bool StreamForPath(const Request*, Response*) = 0;
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
    bool StreamForPath(const Request*, Response*);
};

/** An HTTP/1.1 web server.
 */
class Server
{
    util::PollerInterface *m_poller;
    util::WorkerThreadPool *m_thread_pool;
    typedef std::list<ContentFactory*> list_t;
    list_t m_content;
    StreamSocketPtr m_server_socket;
    unsigned short m_port;
    TaskPoller m_task_poller;

    class Task;
    friend class Task;

public:
    explicit Server(util::PollerInterface*, util::WorkerThreadPool*);
    ~Server();

    /** Pass port==0 to get a random unassigned port
     */
    unsigned Init(unsigned short port = 0);

    unsigned short GetPort();

    /** Mounts a virtual filesystem on the given "mount point".
     *
     * Note that cf->StreamForPath should also check against page_root, as
     * it may be handed requests for other pages. This also lets you mount
     * the same ContentFactory at several mount-points.
     */
    void AddContentFactory(const std::string& page_root, ContentFactory *cf);

    void StreamForPath(const Request*, Response*);

    // Being a Pollable
    unsigned OnActivity();
};

} // namepace http

} // namespace util

#endif
