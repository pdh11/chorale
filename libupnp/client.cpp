#include "client.h"
#include "libutil/trace.h"
#include "libutil/http_fetcher.h"
#include "libutil/http_stream.h"
#include "libutil/http_server.h"
#include "libutil/socket.h"
#include "libutil/string_stream.h"
#include "libutil/xml.h"
#include "libutil/xmlescape.h"
#include "libupnp/description.h"
#include "libupnp/soap.h"
#include <boost/format.hpp>

LOG_DECL(UPNP);

namespace upnp {

class DeviceClient::Impl: public util::http::ContentFactory
{
    util::http::Client *m_client;
    util::http::Server *m_server;

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

public:
    Impl(util::http::Client *client, util::http::Server *server)
	: m_client(client),
	  m_server(server),
	  m_gena_callback((boost::format("/upnp/gena/%u")
			   % (sm_gena_generation++)).str())
    {
	server->AddContentFactory(m_gena_callback, this);
    }

    util::http::Client *GetHttpClient() { return m_client; }
    void SetLocalIPAddress(util::IPAddress a) { m_local_address = a; }

    unsigned int RegisterClient(const char *service_id, ServiceClient*);
    void UnregisterClient(const char *service_id, ServiceClient*);

    unsigned int SoapAction(const char *service_id, const char *action_name,
			    const soap::Outbound& in, soap::Inbound *result);

    // Being a ContentFactory
    bool StreamForPath(const util::http::Request*, util::http::Response*);
};

unsigned DeviceClient::Impl::sm_gena_generation = 0;

DeviceClient::DeviceClient(util::http::Client *client,
			   util::http::Server *server)
    : m_impl(new Impl(client, server))
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

//    TRACE << "descurl " << descurl << "\n";
//    TRACE << "description " << description << "\n";

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

const std::string& DeviceClient::GetFriendlyName() const
{
    return m_impl->m_desc.GetFriendlyName();
}

class DeviceClient::Impl::EventXMLObserver: public xml::SaxParserObserver,
					    public util::BufferSink
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

    // Being a BufferSink
    unsigned int OnBuffer(util::BufferPtr p)
    {
//	TRACE << "I think I've got a buffer, size " << (p ? p->actual_len : 0)
//	      << " used " << p.start << "--" << (p.start+p.len) << "\n";
	return m_parser.OnBuffer(p);
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
    }

    if (i == m_services.end())
	m_services[service_id] = sc;
    return 0;
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
	
    util::http::StreamPtr hsp;
    unsigned int rc = util::http::Stream::Create(&hsp, 
						 i->second.control_url.c_str(),
						 headers.c_str(),
						 body.c_str());
    if (rc)
    {
	TRACE << "Failed to soap\n";
	return rc;
    }

    SoapXMLObserver sxo(result);
    xml::SaxParser parser(&sxo);
    rc = parser.Parse(hsp);
    if (rc)
    {
	TRACE << "Failed to parse\n";
	return rc;
    }

    return 0;
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

} // namespace upnp
