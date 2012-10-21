#ifndef LIBUTIL_HTTP_SERVER_H
#define LIBUTIL_HTTP_SERVER_H 1

#include <map>
#include <list>
#include <memory>
#include <string>
#include <string.h>
#include <boost/noncopyable.hpp>
#include "ip.h"

namespace util {

class IPFilter;
class Scheduler;
class WorkerThreadPool;
class Stream;

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
    IPEndPoint local_ep; ///< The IP/port on which the client found us
    unsigned int access; ///< As per util::IPFilter
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

    Request()
        : refresh(false), has_body(false), access(0) {}
};

/** Filled-in by the ContentFactory, passed back to the http::Server.
 */
struct Response: public boost::noncopyable
{
    /** A place to send the incoming body (for POST or similar).
     *
     * The stream's Write() method will be called as the body comes
     * in; the Read() method is never called. The Write() method gets
     * called with 0 bytes to mean successful end of input.
     */
    std::auto_ptr<util::Stream> body_sink;

    /** The stream to return to the client as the outgoing body.
     */
    std::auto_ptr<util::Stream> body_source;

    /** The HTTP Content-Type to return; if left NULL, "text/html" is used.
     */
    const char *content_type;

    /** The HTTP status line to return; if left NULL, an "HTTP/1.1 200
     * OK" line is used if ssp is non-NULL, a 404 one otherwise.
     */
    const char *status_line;

    /** Any additional HTTP headers to return.
     */
    std::map<std::string, std::string> headers;

    /** Body length (Content-Length) to return.
     *
     * If zero, ssp->GetLength() is used (which is usually what you want).
     */
    uint64_t length;

    void Clear();

    Response()
        : content_type(NULL), status_line(NULL), length(0) {}
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
    ~FileContentFactory();

    // Being a ContentFactory
    bool StreamForPath(const Request*, Response*);
};

/** An HTTP/1.1 web server.
 */
class Server
{
    util::Scheduler *m_scheduler;
    util::WorkerThreadPool *m_pool;
    util::IPFilter *m_filter;
    typedef std::list<ContentFactory*> list_t;
    list_t m_content;
    unsigned short m_port;
    std::string m_server_header;

    class DataTask;
    friend class DataTask;

    class AcceptorTask;
    friend class AcceptorTask;

public:
    Server(util::Scheduler*, util::WorkerThreadPool*, util::IPFilter* = NULL);
    ~Server();

    /** Pass port==0 to get a random unassigned port
     */
    unsigned Init(unsigned short port = 0);

    unsigned short GetPort();

    const std::string& GetServerHeader() { return m_server_header; }

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
