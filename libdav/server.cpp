#include "server.h"
#include "libutil/trace.h"
#include "libutil/file.h"
#include "libutil/file_stream.h"
#include "libutil/string_stream.h"
#include "libutil/urlescape.h"
#include "libutil/xmlescape.h"
#include "libutil/xml.h"
#include "libutil/errors.h"
#include "libutil/counted_pointer.h"
#include <boost/format.hpp>
#include <time.h>
#include <string.h>
#include <stdio.h>

LOG_DECL(DAV);

namespace dav {

static std::string RFC3339Date(time_t t)
{
    char buffer[40];

    struct tm stm;

#if HAVE_GMTIME_R
    gmtime_r(&t, &stm);
#else
    /* Win32 gmtime() is in fact thread-safe because it returns TLS, says:
     * http://msdn.microsoft.com/en-us/library/bf12f0hc%28VS.80%29.aspx
     */
    stm = *gmtime(&t);
#endif
    
    strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &stm);
    return buffer;
}

enum {
    RESOURCE_TYPE = 0x1,
    CREATION_DATE = 0x2,
    GET_CONTENT_LENGTH = 0x4,
    GET_CONTENT_TYPE = 0x8,
    GET_LAST_MODIFIED = 0x10,

    ALLPROP = 0xFF
};

static const struct {
    const char *tag;
    unsigned int property;
} properties[] = {
    { "resourcetype",     RESOURCE_TYPE },
    { "creationdate",     CREATION_DATE },
    { "getcontentlength", GET_CONTENT_LENGTH },
    { "getcontenttype",   GET_CONTENT_TYPE },
    { "getlastmodified",  GET_LAST_MODIFIED },
    { "allprop",          ALLPROP },
};

static std::string PropStat(const struct stat *st, unsigned int which,
			    const std::string& nonexistent)
{
    std::string reply = "<propstat><prop>";

    if (which & RESOURCE_TYPE)
    {
	if (S_ISDIR(st->st_mode))
	    reply += "<resourcetype><collection/></resourcetype>";
	else
	    reply += "<resourcetype/>";
    }

    if (which & CREATION_DATE)
    {
	reply += (boost::format("<creationdate>%s</creationdate>")
		  % RFC3339Date(st->st_ctime)).str();
    }

    if (which & GET_LAST_MODIFIED)
    {
	reply += (boost::format("<getlastmodified>%s</getlastmodified>")
		  % RFC3339Date(st->st_mtime)).str();
    }

    if (which & GET_CONTENT_LENGTH && !S_ISDIR(st->st_mode))
    {
	reply += (boost::format("<getcontentlength>%u</getcontentlength>")
		  % st->st_size).str();
    }

    if (which & GET_CONTENT_TYPE && !S_ISDIR(st->st_mode))
    {
	reply += (boost::format("<getcontenttype>%u</getcontenttype>")
		  % "text/html").str();
    }

    reply += "</prop>"
	    " <status>HTTP/1.1 200 OK</status>"
	    " </propstat>";

    if (!nonexistent.empty())
    {
	reply += "<propstat><prop>" + nonexistent + "</prop>"
	    "<status>HTTP/1.1 404 Not Found</status>"
	    "</propstat>";
    }

    return reply;
}

class Properties
{
    std::string m_href;
    std::string m_path;
    unsigned int m_which;
    std::string m_nonexistent;
    unsigned int m_depth;

public:
    Properties(const std::string& href, const std::string& path,
	       unsigned int which_properties,
	       const std::string& nonexistent_properties, unsigned int depth)
	: m_href(href),
	  m_path(path),
	  m_which(which_properties),
	  m_nonexistent(nonexistent_properties),
	  m_depth(depth)
    {
    }

    std::string ToString();
};

std::string Properties::ToString()
{
    std::string reply = 
	"<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
	"<multistatus xmlns=\"DAV:\">";

    struct stat st;
    stat(m_path.c_str(), &st);

    std::string href = m_href;

    if (S_ISDIR(st.st_mode) && href[href.length()-1] != '/')
	href += "/";

    reply += "<response><href>" + util::XmlEscape(href) + "</href>";
    reply += PropStat(&st, m_which, m_nonexistent);
    reply += "</response>";

    if (m_depth > 0 && S_ISDIR(st.st_mode))
    {
	std::vector<util::Dirent> entries;

	unsigned rc = util::ReadDirectory(m_path, &entries);
	if (rc)
	{
	    TRACE << "Resolved dir'" << m_path << "' won't read " << rc
		  << "\n";
	}
	else
	{
	    for (std::vector<util::Dirent>::const_iterator i = entries.begin();
		 i != entries.end();
		 ++i)
	    {
		if (i->name[0] == '.')
		    continue;

		std::string href2 = href + i->name;
		if (S_ISDIR(i->st.st_mode))
		    href2 += "/";

		reply += "<response><href>" + util::XmlEscape(href2) + "</href>";
		reply += PropStat(&i->st, m_which, m_nonexistent);
		reply += "</response>";
	    }
	}
    }
	
    reply += "</multistatus>";
    return reply;
}

class PropFindReplier: public util::Stream,
		       public xml::SaxParserObserver
{
    std::string m_href;
    std::string m_path;
    util::http::Response *m_response;
    unsigned int m_depth;
    xml::SaxParser m_parser;
    unsigned int m_which_properties;
    std::string m_nonexistent_properties;
    bool m_in_props;

public:
    PropFindReplier(const std::string& href, const std::string& path,
		    util::http::Response *response,
		    unsigned int depth)
	: m_href(href),
	  m_path(path),
	  m_response(response),
	  m_depth(depth),
	  m_parser(this),
	  m_which_properties(0),
	  m_nonexistent_properties(),
	  m_in_props(false)
    {
	TRACE << "PFR\n";
    }

    ~PropFindReplier()
    {
	TRACE << "~PFR()\n";
    }

    // Being an xml::SaxParserObserver
    unsigned int OnBegin(const char *tag)
    {
	if (!strcasecmp(tag, "PROP"))
	    m_in_props = true;
	else if (m_in_props)
	{
	    LOG(DAV) << "Prop: <" << tag << ">\n";
	
	    for (unsigned int i=0;
		 i<sizeof(properties)/sizeof(properties[0]); 
		 ++i)
	    {
		if (!strcasecmp(properties[i].tag, tag))
		{
		    m_which_properties |= properties[i].property;
		    return 0;
		}
	    }

	    m_nonexistent_properties += std::string("<") + tag + "/>";
	}
	
	return 0;
    }

    // Being a Stream
    unsigned GetStreamFlags() const { return WRITABLE; }
    unsigned Read(void *, size_t, size_t*) { return EPERM; }
    unsigned Write(const void *buffer, size_t len, size_t *pwrote)
    {
	TRACE << "PFR::Write(" << len << ")\n";

	if (len)
	{
	    unsigned int rc = m_parser.WriteAll(buffer, len);
	    if (!rc)
		*pwrote = len;
	    return rc;
	}
	else
	{
	    // len==0 means EOF
	    Properties p(m_href, m_path, m_which_properties,
			 m_nonexistent_properties, m_depth);

	    std::string reply = p.ToString();

	    TRACE << "PROPFIND reply " << reply << "\n";

	    m_response->body_source.reset(new util::StringStream(reply));
	    m_response->content_type = "application/xml; charset=\"utf-8\"";
	    m_response->status_line = "HTTP/1.1 207 Multi-Status\r\n";
	    m_in_props = false;
	    return 0;
	}
    }
};

Server::Server(const std::string& file_root,
	       const std::string& page_root)
    : m_page_root(page_root)
{
    TRACE << "dav::Server\n";
    m_file_root = util::Canonicalise(file_root);
}

bool Server::StreamForPath(const util::http::Request *rq, 
			   util::http::Response *rs)
{
    TRACE << "Got " << rq->verb << " request for page '" << rq->path << "'\n";

    if (!util::IsInRoot(m_page_root.c_str(), rq->path.c_str()))
    {
	TRACE << "Not in my root '" << m_page_root << "'\n";
	return false;
    }

    if (rq->verb == "OPTIONS")
    {
	rs->body_source.reset(new util::StringStream());
	rs->headers["DAV"] = "1";
	return true;
    }

    std::string path2 = m_file_root
	+ std::string(rq->path, m_page_root.length());

    TRACE << "path2a=" << path2 << "\n";

    path2 = util::Canonicalise(path2);

    TRACE << "path2b=" << path2 << "\n";

    // Make sure it's still under the right root (no "/../" attacks)
    if (!util::IsInRoot(m_file_root.c_str(), path2.c_str()))
    {
	TRACE << "Resolved file '" << path2 << "' not in my root '" 
	      << m_file_root << "'\n";
	return false;
    }

    TRACE << "IsInRoot2\n";

    bool creative = (rq->verb == "MKCOL" || rq->verb == "PUT");

    struct stat st;
    bool found = stat(path2.c_str(), &st) == 0;
    if (!found && !creative)
    {
	LOG(DAV) << "Resolved file '" << path2 << "' not found\n";
	return false;
    }

    LOG(DAV) << "Path '" << rq->path << "' is file '" << path2 << "'\n";

    if (rq->verb == "GET")
    {
	if (!S_ISREG(st.st_mode))
	{
	    TRACE << "Resolved file '" << path2 << "' not a file\n";
	    return false;
	}

	unsigned int rc = util::OpenFileStream(path2.c_str(), util::READ,
					       &rs->body_source);
	if (rc != 0)
	{
	    TRACE << "Resolved file '" << path2 << "' won't open " << rc
		  << "\n";
	    return false;
	}
    }
    else if (rq->verb == "PROPFIND")
    {
	std::string sdepth = rq->GetHeader("DEPTH");
	unsigned int depth;

	if (sdepth == "" || sdepth == "infinity")
	    depth = 2;
	else
	    depth = atoi(sdepth.c_str());

	TRACE << "depth " << depth << "\n";

	if (rq->has_body)
	{
	    rs->content_type = "text/xml; charset=\"utf-8\"";
	    rs->body_sink.reset(new PropFindReplier(rq->path, path2, rs,
						    depth));
	    return true;
	}

	// No body means ALLPROP

	Properties p(rq->path, path2, ALLPROP, std::string(), depth);

	std::string reply = p.ToString();
	
	LOG(DAV) << "Response: " << reply << "\n";

	rs->body_source.reset(new util::StringStream(reply));
	rs->content_type = "application/xml; charset=\"utf-8\"";
	rs->status_line = "HTTP/1.1 207 Multi-Status\r\n";
	return true;
    }
    else if (rq->verb == "MKCOL")
    {
	if (found)
	{
	    rs->status_line = "HTTP/1.1 405 Not Allowed\r\n";
	    return true;
	}

	unsigned int rc = util::Mkdir(path2.c_str());
	if (rc)
	    rs->status_line = "HTTP/1.1 409 Conflict\r\n";
	else
	{
	    rs->status_line = "HTTP/1.1 201 Created\r\n";
	    rs->body_source.reset(new util::StringStream(""));
	}
	return true;
    }
    else if (rq->verb == "PUT")
    {
	if (found && !S_ISREG(st.st_mode))
	{
	    rs->status_line = "HTTP/1.1 409 Conflict\r\n";
	    return true;
	}

	TRACE << "Got a PUT, headers: " << rq->headers;

	unsigned int rc = util::OpenFileStream(path2.c_str(),
					       util::WRITE | util::SEQUENTIAL,
					       &rs->body_sink);
	if (rc)
	{
	    rs->status_line = "HTTP/1.1 405 Not Allowed\r\n";
	    return true;
	}

	rs->status_line = found ? "HTTP/1.1 200 OK\r\n"
                                : "HTTP/1.1 201 Created\r\n";
	rs->body_source.reset(new util::StringStream(""));
	return true;
    }
    else if (rq->verb == "DELETE")
    {
	if (!S_ISREG(st.st_mode))
	{
	    rs->status_line = "HTTP/1.1 405 Not Allowed\r\n";
	    return true;
	}

	int res = ::remove(path2.c_str());
	if (res < 0)
	    rs->status_line = "HTTP/1.1 405 Not Allowed\r\n";
	else
	{
	    rs->status_line = "HTTP/1.1 204 No Content\r\n";
	    rs->body_source.reset(new util::StringStream(""));
	}
	return true;
    }

    return true;
}

} // namespace dav

#ifdef TEST

# include <fcntl.h>
# include <stdlib.h>
# include "libutil/scheduler.h"
# include "libutil/worker_thread_pool.h"
# include "libutil/http_fetcher.h"
# include "libutil/http_client.h"
# include "libutil/bind.h"
# include <boost/regex.hpp>

void CheckMatch(const char *filename, const char *pattern)
{
    std::auto_ptr<util::Stream> stm;
    unsigned int rc = util::OpenFileStream(filename, util::READ,
					   &stm);
    if (rc)
	fprintf(stderr, "Can't open output file '%s'\n", filename);
    assert(rc == 0);

    util::StringStream sstm;

    rc = util::CopyStream(stm.get(), &sstm);
    if (rc)
	fprintf(stderr, "Can't read output file: %u\n", rc);
    assert(rc == 0);

    boost::regex e(pattern, boost::regex::basic);

    if (!boost::regex_match(sstm.str(), e))
    {
	fprintf(stderr, "Output file\n%s\ndoesn't match pattern\n%s\n",
		sstm.str().c_str(), pattern);
	assert(0);
    }
}

void TryDav(const std::string& url, const char *script, const char *pattern)
{
#if HAVE_CADAVER && HAVE_MKSTEMP
    char outfile[] = "dav.server.out.XXXXXX";

    int outfd = mkstemp(outfile);
    if (!outfd)
    {
	TRACE << "Can't create output file\n";
	exit(1);
    }

    std::string command = "cadaver " + url + " >&"
	+ (boost::format("%d") % outfd).str();

    /* --- */

    FILE *f = popen(command.c_str(), "w");
    fprintf(f, script);
    int rc = pclose(f);

    TRACE << "pclose returned " << rc << "\n";
    assert(rc == 0);

    CheckMatch(outfile, pattern);

    close(outfd);
    unlink(outfile);
#endif
}

int main(int, char *[])
{
    util::WorkerThreadPool server_threads(util::WorkerThreadPool::NORMAL, 4);

    util::http::Client hc;
    util::BackgroundScheduler poller;
    util::http::Server ws(&poller,&server_threads);

    server_threads.PushTask(util::SchedulerTask::Create(&poller));

    unsigned rc = ws.Init();

    char root[] = "dav.server.test.XXXXXX";

#ifndef WIN32

    if (!mkdtemp(root))
    {
	fprintf(stderr, "Can't create temporary dir\n");
	return 1;
    }

    std::string fullname = util::Canonicalise(root);

    std::string file = fullname + "/ptang.txt";
    FILE *f = fopen(file.c_str(), "wb");
    fprintf(f, "frink\n");
    fclose(f);

    dav::Server davs(fullname.c_str(), "/zootle");

    ws.AddContentFactory("/zootle", &davs);

    assert(rc == 0);

    std::string url = (boost::format("http://127.0.0.1:%u/zootle/")
		       % ws.GetPort()
	).str();

    util::http::Fetcher fetch(&hc, url, "Depth: 0\r\n", NULL, "PROPFIND");

    std::string result;
    rc = fetch.FetchToString(&result);
    assert(rc == 0);

    TRACE << result << "\n";

#if HAVE_CADAVER

    TryDav(url, "ls\n",
	   ".*dav:/zootle/> ls\n"
	   "Listing collection .* succeeded.\n"
	   ".*ptang.txt.*6.*\n"
	   "dav:/zootle/> \n"
	   ".*\n"
	);

    TryDav(url, "mkcol /zootle/frink\n",
	   ".*dav:/zootle/> mkcol /zootle/frink\n"
	   "Creating .* succeeded.\n"
	   "dav:/zootle/> \n"
	   ".*\n"
	);

    std::vector<util::Dirent> entries;
    rc = util::ReadDirectory(root, &entries);
    assert(rc == 0);
    assert(entries.size() == 4);

    char upload[] = "dav.server.upload.XXXXXX";
    int upfd =  mkstemp(upload);
    if (!upfd)
    {
	TRACE << "Can't create upload file\n";
	return 1;
    }

    rc = (unsigned)::write(upfd, "wurdlefoo\n", 10);
    assert(rc == 10);

    std::string script = std::string("put ") + upload + " /zootle/frink/aubergine.mp3\n";

    TryDav(url, script.c_str(),
	   ".*dav:/zootle/> put .* /zootle/frink/aubergine.mp3\n"
	   "Uploading .* to .* succeeded.\n"
	   "dav:/zootle/> \n"
	   ".*\n"
	);

    unlink(upload);
    close(upfd);

    CheckMatch((fullname + "/frink/aubergine.mp3").c_str(), "wurdlefoo\n");

    TryDav(url, "delete /zootle/frink/aubergine.mp3",
	   ".*dav:/zootle/> delete /zootle/frink/aubergine.mp3\n"
	   "Deleting .* succeeded.\n"
	   "dav:/zootle/> \n"
	   ".*\n"
	);

    struct stat st;
    rc = ::stat((fullname + "/frink/aubergine.mp3").c_str(), &st);
    assert(rc != 0);

#endif // HAVE_CADAVER

    std::string rmrf = "rm -r " + std::string(root);
    rc = system(rmrf.c_str());
    assert(rc == 0);
    
#endif // WIN32

    return 0;
}

#endif
