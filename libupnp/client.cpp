#include "client.h"
#include "libutil/trace.h"
#include "libutil/errors.h"
#include "libutil/http_fetcher.h"
#include "libutil/http_server.h"
#include "libutil/http_client.h"
#include "libutil/socket.h"
#include "libutil/string_stream.h"
#include "libutil/xml.h"
#include "libutil/xmlescape.h"
#include "libutil/scheduler.h"
#include "libupnp/description.h"
#include "libupnp/soap.h"
#include <boost/format.hpp>

LOG_DECL(UPNP);

namespace upnp {

class DeviceClient::Impl: public util::http::ContentFactory
{
    util::http::Client *m_client;
    util::http::Server *m_server;
    util::Scheduler *m_scheduler;

    upnp::Description m_desc;

    typedef std::map<std::string, ServiceClient*> sidmap_t;
    sidmap_t m_sidmap;

    typedef std::map<const char*, ServiceClient*> servicemap_t;
    servicemap_t m_services;

    std::string m_gena_callback;

    /** Local IP address from which we contacted this device.
     */
    util::IPAddress m_local_address;

    static unsigned sm_gena_generation;

    friend class DeviceClient;

    class EventXMLObserver;
    class SoapXMLObserver;
    class Response;
    class AsyncSoapHandler;
    class AsyncInitHandler;
    class AsyncSubscribeHandler;
    class SyncSoapHandler;

public:
    Impl(util::http::Client *client, util::http::Server *server,
	 util::Scheduler *scheduler)
	: m_client(client),
	  m_server(server),
	  m_scheduler(scheduler),
	  m_gena_callback((boost::format("/upnp/gena/%u")
			   % (sm_gena_generation++)).str())
    {
	server->AddContentFactory(m_gena_callback, this);
    }

    void SetLocalIPAddress(util::IPAddress a) { m_local_address = a; }

    unsigned int RegisterClient(const char *service_id, ServiceClient*);
    unsigned int RegisterClient(const char *service_id, ServiceClient*,
				ServiceClient::InitCallback);
    void UnregisterClient(const char *service_id, ServiceClient*);

    /** Synchronous SOAP */
    unsigned int SoapAction(const char *service_id, const char *action_name,
			    const soap::Outbound& in, soap::Inbound *result);

    /** Asynchronous SOAP */
    unsigned int SoapAction(const char *service_id, const char *action_name,
			    const soap::Outbound& in, 
			    ServiceClient::SoapCallback callback);

    // Being a ContentFactory
    bool StreamForPath(const util::http::Request*, util::http::Response*);
};

unsigned DeviceClient::Impl::sm_gena_generation = 0;

DeviceClient::DeviceClient(util::http::Client *client,
			   util::http::Server *server,
			   util::Scheduler *scheduler)
    : m_impl(new Impl(client, server, scheduler))
{
}

DeviceClient::~DeviceClient()
{
    delete m_impl;
}

unsigned int DeviceClient::Init(const std::string& descurl,
				const std::string& udn)
{
    util::http::Fetcher fetcher(m_impl->m_client, descurl);

    std::string description;
    unsigned int rc = fetcher.FetchToString(&description);
    if (rc)
    {
	TRACE << "Can't fetch description: " << rc << "\n";
	return rc;
    }

    LOG(UPNP) << "descurl " << descurl << "\n";
    LOG(UPNP) << "description " << description << "\n";

    rc = m_impl->m_desc.Parse(description, descurl, udn);
    if (rc)
    {
	TRACE << "Can't parse description\n";
	return rc;
    }

    util::IPEndPoint ipe = fetcher.GetLocalEndPoint();
    m_impl->SetLocalIPAddress(ipe.addr);

    /// @todo Reattach all services on reinit, as they need to resubscribe

    return 0;
}

class DeviceClient::Impl::AsyncInitHandler: public util::http::Connection
{
    DeviceClient::Impl *m_dci;
    DeviceClient::InitCallback m_callback;
    std::string m_description;
    std::string m_descurl;
    std::string m_udn;

public:
    AsyncInitHandler(DeviceClient::Impl *dci, const std::string& descurl,
		     const std::string& udn,
		     DeviceClient::InitCallback callback)
	: m_dci(dci), m_callback(callback), m_descurl(descurl), m_udn(udn)
    {
    }

    // Being a util::http::Connection
    unsigned Write(const void*, size_t, size_t*);
    void OnEndPoint(const util::IPEndPoint&);
    void OnDone(unsigned int rc);
};

unsigned DeviceClient::Impl::AsyncInitHandler::Write(const void *buffer,
						     size_t len,
						     size_t *nwrote)
{
    m_description.append((const char*)buffer, len);
    *nwrote = len;
    return 0;
}

void DeviceClient::Impl::AsyncInitHandler::OnEndPoint(const util::IPEndPoint& endpoint)
{
    m_dci->SetLocalIPAddress(endpoint.addr);
}

void DeviceClient::Impl::AsyncInitHandler::OnDone(unsigned int rc)
{
//    TRACE << "aih" << this << " in OnDone(" << rc << ")\n";
    if (!rc)
    {
//	TRACE << "Description: " << m_description << "\n";
	rc = m_dci->m_desc.Parse(m_description, m_descurl, m_udn);
    }

//    TRACE << "aih" << this << " calls callback\n";
    m_callback(rc);
}

unsigned int DeviceClient::Init(const std::string& descurl,
				const std::string& udn,
				DeviceClient::InitCallback callback)
{
    util::http::ConnectionPtr hcp(new Impl::AsyncInitHandler(m_impl, 
							     descurl, udn,
							     callback));

    unsigned int rc = m_impl->m_client->Connect(m_impl->m_scheduler, hcp,
						descurl);
    if (rc)
	TRACE << "Failed to init connection";
//    TRACE << "HTTP running asynchronously\n";

    return rc;
}

const std::string& DeviceClient::GetFriendlyName() const
{
    return m_impl->m_desc.GetFriendlyName();
}

class DeviceClient::Impl::EventXMLObserver: public util::Stream,
					    public xml::SaxParserObserver
{
    std::string m_key, m_value;
    ServiceClient *m_connection;
    xml::SaxParser m_parser;

public:
    explicit EventXMLObserver(ServiceClient *connection)
	: m_connection(connection),
	  m_parser(this)
    {
    }

    unsigned int OnBegin(const char *tag)
    {
	LOG(UPNP) << "Event: <" << tag << ">\n";
	if (!strchr(tag, ':'))
	{
	    m_key = tag;
	    m_value.clear();
	}
	else
	    m_key.clear();
	return 0;
    }

    unsigned int OnContent(const char *content)
    {
	if (!m_key.empty())
	    m_value += content;
	return 0;
    }

    unsigned int OnEnd(const char*)
    {
	if (!m_key.empty())
	{
	    LOG(UPNP) << "GENA Event '" << m_key << "' = '" << m_value
		      << "'\n";
	    m_connection->OnEvent(m_key.c_str(), m_value);
	    m_key.clear();
	    m_value.clear();
	}
	return 0;
    }

    // Being a Stream
    unsigned Read(void *, size_t, size_t*) { return EPERM; }
    unsigned Write(const void *buffer, size_t len, size_t *pwrote)
    {
	unsigned rc = m_parser.WriteAll(buffer, len);
	if (!rc)
	    *pwrote = len;
	return rc;
    }
};

bool DeviceClient::Impl::StreamForPath(const util::http::Request *rq, 
				       util::http::Response *rs)
{
//    TRACE << "In SfP\n";
    if (rq->path == m_gena_callback)
    {
	std::map<std::string, std::string>::const_iterator ci
	    = rq->headers.find("SID");
	if (ci != rq->headers.end())
	{
	    std::string sid = ci->second;
	    LOG(UPNP) << "mine " << rq->verb << " sid=" << sid << "\n";

	    sidmap_t::const_iterator cii = m_sidmap.find(sid);
	    if (cii != m_sidmap.end())
	    {
		ServiceClient *connection = cii->second;
//		TRACE << "Connection=" << connection << "\n";

		rs->body_sink.reset(new EventXMLObserver(connection));
		rs->ssp = util::StringStream::Create("");
		return true;
	    }
	}
    }
    return false;
}

unsigned int DeviceClient::Impl::RegisterClient(const char *service_id,
						ServiceClient *sc)
{
    servicemap_t::const_iterator i = m_services.find(service_id);
    if (i != m_services.end() && i->second != sc)
    {
	TRACE << "ServiceClient already registered for " << service_id << "\n";
	return EEXIST;
    }

    const upnp::Services& svc = m_desc.GetServices();

    upnp::Services::const_iterator it = svc.find(service_id);
    if (it == svc.end())
    {
	TRACE << "No service with id '" << service_id << "'\n";
	return ENOENT;
    }

    util::IPEndPoint local_endpoint;
    local_endpoint.addr = m_local_address;
    local_endpoint.port = m_server->GetPort();

    std::string headers = "Callback: <http://"
	+ local_endpoint.ToString()
	+ m_gena_callback + ">\r\n"
	"NT: upnp:event\r\n"
	"Timeout: Second-7200\r\n";

    LOG(UPNP) << "GENA subscribe:\n" << headers;

    util::http::Fetcher fetcher(m_client,
				it->second.event_url,
				headers.c_str(),
				NULL,
				"SUBSCRIBE");

    std::string result;
    unsigned int rc = fetcher.FetchToString(&result);
    if (rc)
    {
	TRACE << "Can't subscribe: " << rc << "\n";
	return rc;
    }
    std::string sid = fetcher.GetHeader("SID");
    if (!sid.empty())
    {
	LOG(UPNP) << "Got SID " << sid << "\n";
	m_sidmap[sid] = sc;
    }
    else
    {
	TRACE << "Can't subscribe, no SID\n";
	return ENOENT;
    }

    if (i == m_services.end())
	m_services[service_id] = sc;
    return 0;
}


class DeviceClient::Impl::AsyncSubscribeHandler: public util::http::Connection
{
    DeviceClient::Impl *m_dci;
    const char *m_service_id;
    ServiceClient *m_client;
    ServiceClient::InitCallback m_callback;
    std::string m_subscription_id;

public:
    AsyncSubscribeHandler(DeviceClient::Impl *dci, 
			  const char *service_id,
			  ServiceClient *client,
			  ServiceClient::InitCallback callback)
	: m_dci(dci),
	  m_service_id(service_id), 
	  m_client(client), 
	  m_callback(callback)
    {
    }

    // Being a util::http::Connection
    void OnHeader(const std::string&, const std::string&);
    void OnDone(unsigned int rc);
};

void DeviceClient::Impl::AsyncSubscribeHandler::OnHeader(const std::string& key, const std::string& value)
{
    if (!strcasecmp(key.c_str(), "SID"))
	m_subscription_id = value;
}

void DeviceClient::Impl::AsyncSubscribeHandler::OnDone(unsigned int rc)
{
//    TRACE << "asubh" << this << " in OnHttpDone(" << rc << ")\n";
    if (!rc && m_subscription_id.empty())
    {
	TRACE << "Can't subscribe, no SID\n";
	rc = EINVAL;
    }
    else if (!rc)
    {
	m_dci->m_sidmap[m_subscription_id] = m_client;
	m_dci->m_services[m_service_id] = m_client;
    }
//    TRACE << "asubh" << this << " calls callback\n";
    m_callback(rc);
}

unsigned int DeviceClient::Impl::RegisterClient(const char *service_id,
						ServiceClient *sc,
						ServiceClient::InitCallback callback)
{
//    TRACE << "In async RegisterClient\n";

    servicemap_t::const_iterator i = m_services.find(service_id);
    if (i != m_services.end() && i->second != sc)
    {
	TRACE << "ServiceClient already registered for " << service_id << "\n";
	return EEXIST;
    }

    const upnp::Services& svc = m_desc.GetServices();

    upnp::Services::const_iterator it = svc.find(service_id);
    if (it == svc.end())
    {
	TRACE << "No service with id '" << service_id << "'\n";
	return ENOENT;
    }

    util::IPEndPoint local_endpoint;
    local_endpoint.addr = m_local_address;
    local_endpoint.port = m_server->GetPort();

    std::string headers = "Callback: <http://"
	+ local_endpoint.ToString()
	+ m_gena_callback + ">\r\n"
	"NT: upnp:event\r\n"
	"Timeout: Second-7200\r\n";

    LOG(UPNP) << "GENA subscribe:\n" << headers;

//    TRACE << "creating asubh\n";

    util::http::ConnectionPtr hcp(new AsyncSubscribeHandler(this, service_id,
							    sc, callback));
    unsigned int rc = m_client->Connect(m_scheduler, hcp,
					it->second.event_url.c_str(),
					headers.c_str(), "",
					"SUBSCRIBE");

    if (rc)
	TRACE << "Failed to init connection";
    return rc;
}

void DeviceClient::Impl::UnregisterClient(const char *service_id,
					  ServiceClient *sc)
{
    servicemap_t::iterator i = m_services.find(service_id);
    if (i != m_services.end() && i->second == sc)
	m_services.erase(i);

    for (sidmap_t::iterator ii = m_sidmap.begin(); ii != m_sidmap.end(); ++ii)
    {
	if (ii->second == sc)
	{
	    m_sidmap.erase(ii);
	    break;
	}
    }
}

class DeviceClient::Impl::SoapXMLObserver: public xml::SaxParserObserver
{
    soap::Inbound *m_params;
    std::string m_tag;
    std::string m_value;

public:
    explicit SoapXMLObserver(soap::Inbound *params) : m_params(params) {}

    unsigned int OnBegin(const char *tag);
    unsigned int OnEnd(const char *tag);
    unsigned int OnContent(const char *content);
};

unsigned int DeviceClient::Impl::SoapXMLObserver::OnBegin(const char *tag)
{
    if (!strchr(tag, ':'))
	m_tag = tag;
    m_value.clear();
    return 0;
}

unsigned int DeviceClient::Impl::SoapXMLObserver::OnContent(const char *content)
{
    if (!m_tag.empty())
	m_value += content;
    return 0;
}

unsigned int DeviceClient::Impl::SoapXMLObserver::OnEnd(const char *tag)
{
    if (!strchr(tag, ':'))
    {
//	TRACE << "Setting " << tag << " to '''" << m_value << "'''\n";
	m_params->Set(tag, m_value);
    }
    m_tag.clear();
    m_value.clear();
    return 0;
}

class DeviceClient::Impl::SyncSoapHandler: public util::http::Connection
{
    xml::SaxParser *m_parser;
    bool m_done;
    unsigned int m_error_code;

public:
    SyncSoapHandler(xml::SaxParser *parser)
	: m_parser(parser),
	  m_done(false),
	  m_error_code(0)
    {
    }

    bool IsDone() const { return m_done; }
    unsigned int GetErrorCode() const { return m_error_code; }

    // Being a util::http::Connection
    unsigned Write(const void*, size_t, size_t*);
    void OnDone(unsigned int rc);
};

unsigned DeviceClient::Impl::SyncSoapHandler::Write(const void *buffer,
						    size_t len, size_t *pwrote)
{
    m_parser->WriteAll((const char*)buffer, len);
    *pwrote = len;
    return 0;
}

void DeviceClient::Impl::SyncSoapHandler::OnDone(unsigned int rc)
{
    if (!m_error_code)
	m_error_code = rc;
    m_done = true;
}

/** Synchronous SOAP.
 * 
 * Uses http::Stream
 */
unsigned int DeviceClient::Impl::SoapAction(const char *service_id,
					    const char *action_name,
					    const soap::Outbound& in,
					    soap::Inbound *result)
{
    const Services& services = m_desc.GetServices();
    Services::const_iterator i = services.find(service_id);
    if (i == services.end())
    {
	TRACE << "Trying to soap non-existent service\n";
	return ENOENT;
    }

    std::string headers = "SOAPACTION: \"";
    headers += i->second.type;
    headers += "#";
    headers += action_name;
    headers += "\"\r\nContent-Type: text/xml; charset=\"utf-8\"\r\n";

    LOG(UPNP) << "Soaping:\n" << headers;

    std::string body = in.CreateBody(action_name, i->second.type);
	
    SoapXMLObserver sxo(result);
    xml::SaxParser parser(&sxo);
    util::CountedPointer<SyncSoapHandler> ptr(new SyncSoapHandler(&parser));
    util::BackgroundScheduler scheduler;

    unsigned int rc = m_client->Connect(&scheduler, ptr,
					i->second.control_url.c_str(),
					headers.c_str(),
					body.c_str());
    if (rc)
	return rc;

    time_t start = time(NULL);
    time_t finish = start+10; // Give it ten seconds
    time_t now;

    do {
	now = time(NULL);
	if (now < finish)
	{
	    rc = scheduler.Poll((unsigned)(finish-now)*1000);
	    if (rc)
		return rc;
	}
    } while (now < finish && !ptr->IsDone() && !ptr->GetErrorCode());
    
    if (!ptr->IsDone())
    {
	rc = ptr->GetErrorCode();
	if (!rc)
	    rc = ETIMEDOUT;
	return rc;
    }
    return 0;
}

class DeviceClient::Impl::AsyncSoapHandler: public util::http::Connection
{
    ServiceClient::SoapCallback m_callback;
    soap::Inbound m_result;
    SoapXMLObserver m_sxo;
    xml::SaxParser m_parser;

public:
    AsyncSoapHandler(ServiceClient::SoapCallback callback)
	: m_callback(callback),
	  m_sxo(&m_result),
	  m_parser(&m_sxo)
    {
    }

    // Being a util::http::Connection
    unsigned Write(const void*, size_t, size_t*);
    void OnDone(unsigned int rc);
};

unsigned DeviceClient::Impl::AsyncSoapHandler::Write(const void *buffer,
						     size_t len,
						     size_t *pwrote)
{
    unsigned int rc = m_parser.WriteAll(buffer, len);
    if (!rc)
	*pwrote = len;
    return rc;
}

void DeviceClient::Impl::AsyncSoapHandler::OnDone(unsigned int rc)
{    
//    TRACE << "ash" << this << ": HTTP done (" << rc << ")\n";
    m_callback(rc, rc==0 ? &m_result : NULL);
}

/** Asynchronous SOAP.
 * 
 * Uses the scheduler and http::Client.
 */
unsigned int DeviceClient::Impl::SoapAction(const char *service_id,
					    const char *action_name,
					    const soap::Outbound& in,
					    ServiceClient::SoapCallback callback)
{
    const Services& services = m_desc.GetServices();
    Services::const_iterator i = services.find(service_id);
    if (i == services.end())
    {
	TRACE << "Trying to soap non-existent service\n";
	return ENOENT;
    }

    std::string headers = "SOAPACTION: \"";
    headers += i->second.type;
    headers += "#";
    headers += action_name;
    headers += "\"\r\nContent-Type: text/xml; charset=\"utf-8\"\r\n";

    LOG(UPNP) << "Soaping:\n" << headers;

    std::string body = in.CreateBody(action_name, i->second.type);

    util::http::ConnectionPtr hcp(new AsyncSoapHandler(callback));

    unsigned int rc = m_client->Connect(m_scheduler, hcp,
					i->second.control_url.c_str(),
					headers.c_str(),
					body.c_str());
    if (rc)
	TRACE << "Failed to init connection";

    return rc;
}


        /* ServiceClient */


ServiceClient::ServiceClient(DeviceClient* parent,
			     const char *service_id)
    : m_parent(parent),
      m_service_id(service_id)
{
}

ServiceClient::~ServiceClient()    
{
    m_parent->m_impl->UnregisterClient(m_service_id, this);
}

unsigned int ServiceClient::Init()
{
    return m_parent->m_impl->RegisterClient(m_service_id, this);
}

unsigned int ServiceClient::Init(InitCallback callback)
{
    return m_parent->m_impl->RegisterClient(m_service_id, this, callback);
}

unsigned int ServiceClient::GenaUInt(const std::string& s)
{
    return (unsigned int)strtoul(s.c_str(), NULL, 10);
}

bool ServiceClient::GenaBool(const std::string& s)
{
    return soap::ParseBool(s);
}

unsigned int ServiceClient::SoapAction(const char *action_name,
					  soap::Inbound *result)
{
    soap::Outbound no_params;
    return SoapAction(action_name, no_params, result);
}

unsigned int ServiceClient::SoapAction(const char *action_name,
				       const soap::Outbound& in,
				       soap::Inbound *result)
{
    return m_parent->m_impl->SoapAction(m_service_id, action_name, in, result);
}

unsigned int ServiceClient::SoapAction(const char *action_name,
				       SoapCallback callback)
{
    soap::Outbound no_params;
    return SoapAction(action_name, no_params, callback);
}

unsigned int ServiceClient::SoapAction(const char *action_name,
				       const soap::Outbound& params,
				       SoapCallback callback)
{
    return m_parent->m_impl->SoapAction(m_service_id, action_name, params,
					callback);
}

} // namespace upnp
