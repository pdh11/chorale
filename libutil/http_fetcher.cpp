#include "http_fetcher.h"
#include "http_client.h"
#include "trace.h"
#include "errors.h"
#include "scheduler.h"
#include "file.h"
#include "counted_pointer.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

namespace util {

namespace http {

class Fetcher::Task: public Recipient
{
    std::map<std::string, std::string> *m_headers;
    unsigned int m_error_code;
    bool m_done;
    std::string m_response;
    IPEndPoint m_local_endpoint;

    // Being a util::http::Recipient
    unsigned OnData(const void *buffer, size_t len);
    void OnHeader(const std::string& /*key*/, const std::string& /*value*/);
    void OnEndPoint(const util::IPEndPoint& endpoint);
    void OnDone(unsigned int error_code);

public:
    explicit Task(std::map<std::string, std::string> *headers);
    ~Task() {}

    bool IsDone() const { return m_done; }
    IPEndPoint GetLocalEndPoint() const { return m_local_endpoint; }
    unsigned int GetErrorCode() const { return m_error_code; }
    const std::string& GetResponse() const { return m_response; }
};

Fetcher::Task::Task(std::map<std::string, std::string> *headers)
    : m_headers(headers),
      m_error_code(0),
      m_done(false)
{
}

void Fetcher::Task::OnHeader(const std::string& key, const std::string& value)
{
//    TRACE << "hf" << this << ": " << key << ": " << value << "\n";
    (*m_headers)[key] = value;
}

void Fetcher::Task::OnEndPoint(const util::IPEndPoint& endpoint)
{
    m_local_endpoint = endpoint;
}

unsigned Fetcher::Task::OnData(const void *buffer, size_t len)
{
    m_response.append((const char*)buffer, len);
    return 0;
}

void Fetcher::Task::OnDone(unsigned int error_code)
{
    if (!m_error_code)
	m_error_code = error_code;
    m_done = true;
}


Fetcher::Fetcher(Client *client, 
		 const std::string& url, const char *extra_headers,
		 const char *body,
		 const char *verb)
    : m_client(client),
      m_url(url),
      m_extra_headers(extra_headers),
      m_body(body),
      m_verb(verb)
{
}

Fetcher::~Fetcher()
{
}

unsigned int Fetcher::FetchToString(std::string *presult)
{
    // Use asynchronous API synchronously by having our own Scheduler
    BackgroundScheduler scheduler;

    CountedPointer<Task> ptr(new Task(&m_headers));

    unsigned int rc = m_client->Connect(&scheduler, ptr, m_url,
					m_extra_headers ? m_extra_headers : "",
					m_body ? m_body : "", m_verb);
    if (rc)
	return rc;

    time_t start = time(NULL);
    time_t finish = start+10; // Give it ten seconds
    time_t now;

    do {
	now = time(NULL);
	if (now < finish)
	{
//	    TRACE << "hf" << this << ": hc" << m_connection << ": polling\n";
	    rc = scheduler.Poll((unsigned)(finish-now)*1000);
	    if (rc)
		return rc;
	}
    } while (now < finish && !ptr->IsDone() && !ptr->GetErrorCode());
    
    if (!ptr->IsDone())
    {
	rc = ptr->GetErrorCode();
	if (!rc)
	{
//	    TRACE << "hf" << this << ": hc" << m_connection << ": timed out\n";
	    rc = ETIMEDOUT;
	}
    }
    else
    {
	m_local_endpoint = ptr->GetLocalEndPoint();
    }

    if (!rc)
	*presult = ptr->GetResponse();
    return rc;
}

} // namespace http

} // namespace util

#ifdef TEST

# include "bind.h"
# include "scheduler.h"
# include "http_server.h"
# include "worker_thread_pool.h"
# include "string_stream.h"
# include <boost/format.hpp>

class EchoContentFactory: public util::http::ContentFactory
{
public:
    bool StreamForPath(const util::http::Request *rq, util::http::Response *rs)
    {
	rs->body_source.reset(new util::StringStream(rq->path));
	return true;
    }
};

int main()
{
    util::WorkerThreadPool server_threads(util::WorkerThreadPool::NORMAL, 4);
    util::BackgroundScheduler poller;
    util::http::Server ws(&poller, &server_threads);
    server_threads.PushTask(util::SchedulerTask::Create(&poller));

    unsigned rc = ws.Init();
    assert(rc == 0);

    EchoContentFactory ecf;
    ws.AddContentFactory("/", &ecf);

    std::string url2 = (boost::format("http://127.0.0.1:%u/zootle/wurdle.html")
		       % ws.GetPort()
	).str();

    util::http::Client hc;
    util::http::Fetcher hf(&hc, url2, "Range: bytes=0-\r\n");
    std::string contents;
    rc = hf.FetchToString(&contents);
    assert(rc == 0);
    assert(contents == "/zootle/wurdle.html");

    return 0;
}

#endif
