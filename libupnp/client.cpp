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

namespace upnp {

class Client::Impl: public util::http::ContentFactory
{
    util::http::Client *m_client;
    util::http::Server *m_server;

    upnp::Description m_desc;

    typedef std::map<std::string, ClientConnection*> sidmap_t;
    sidmap_t m_sidmap;

    std::string m_gena_callback;

    /** Local IP address from which we contacted this device.
     */
    util::IPAddress m_local_address;

    static unsigned sm_gena_generation;

    friend class Client;

    class EventXMLObserver;
    class Response;

public:
    Impl(util::http::Client *client, util::http::Server *server)
	: m_client(client),
	  m_server(server),
	  m_gena_callback((boost::format("/gena-%u")
			   % (sm_gena_generation++)).str())
    {
	server->AddContentFactory(m_gena_callback, this);
    }

    const upnp::Description& GetDescription() { return m_desc; }
    util::http::Client *GetHttpClient() { return m_client; }
    util::http::Server *GetHttpServer() { return m_server; }
    const std::string& GetGenaCallback() { return m_gena_callback; }
    void SetLocalIPAddress(util::IPAddress a) { m_local_address = a; }
    util::IPAddress GetLocalIPAddress() { return m_local_address; }

    void AddListener(ClientConnection *conn, const std::string& sid)
    {
	m_sidmap[sid] = conn;
    }
    void RemoveListener(ClientConnection*);

    // Being a ContentFactory
    bool StreamForPath(const util::http::Request*, util::http::Response*);
};

unsigned Client::Impl::sm_gena_generation = 0;

Client::Client(util::http::Client *client,
	       util::http::Server *server)
    : m_impl(new Impl(client, server))
{
}

Client::~Client()
{
    delete m_impl;
}

unsigned int Client::Init(const std::string& descurl, const std::string& udn)
{
    util::http::Fetcher fetcher(m_impl->GetHttpClient(), descurl);

    std::string description;
    unsigned int rc = fetcher.FetchToString(&description);
    if (rc)
    {
	TRACE << "Can't fetch description: " << rc << "\n";
	return rc;
    }

    TRACE << "descurl " << descurl << "\n";
    TRACE << "description " << description << "\n";

    rc = m_impl->m_desc.Parse(description, descurl, udn);
    if (rc)
    {
	TRACE << "Can't parse description\n";
	return rc;
    }

    util::IPEndPoint ipe = fetcher.GetLocalEndPoint();
    m_impl->SetLocalIPAddress(ipe.addr);

    return 0;
}

const Description& Client::GetDescription() const
{
    return m_impl->m_desc;
}

class Client::Impl::EventXMLObserver: public xml::SaxParserObserver,
				      public util::BufferSink
{
    std::string m_key, m_value;
    ClientConnection *m_connection;
    xml::SaxParser m_parser;

public:
    explicit EventXMLObserver(ClientConnection *connection)
	: m_connection(connection),
	  m_parser(this)
    {
    }

    unsigned int OnBegin(const char *tag)
    {
//	TRACE << "Event: <" << tag << ">\n";
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
//	    TRACE << "Event '" << m_key << "' = '" << m_value << "'\n";
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

bool Client::Impl::StreamForPath(const util::http::Request *rq, 
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
//	    TRACE << "mine " << rq->verb << " sid=" << sid << "\n";

	    sidmap_t::const_iterator cii = m_sidmap.find(sid);
	    if (cii != m_sidmap.end())
	    {
		ClientConnection *connection = cii->second;
//		TRACE << "Connection=" << connection << "\n";

		rs->body_sink.reset(new EventXMLObserver(connection));
		rs->ssp = util::StringStream::Create("");
		return true;
	    }
	}
    }
    return false;
}


        /* ClientConnection */


struct ClientConnection::Impl
{
    Client::Impl *parent;
    ClientConnection *connection;
    const char *service_type;
    std::string control_url;
    std::string sid;
};

ClientConnection::ClientConnection()
    : m_impl(NULL)
{
}


ClientConnection::~ClientConnection()    
{
    delete m_impl;
}

unsigned int ClientConnection::Init(Client *parent, const char *service_type)
{
    if (!parent || !parent->m_impl || m_impl)
	return EINVAL;

    const upnp::Services& svc = parent->m_impl->GetDescription().GetServices();

    upnp::Services::const_iterator it = svc.find(service_type);
    if (it == svc.end())
    {
	TRACE << "No service of type '" << service_type << "'\n";
	return ENOENT;
    }

    m_impl = new Impl;
    m_impl->parent = parent->m_impl;
    m_impl->connection = this;
    m_impl->control_url = it->second.control_url;
    m_impl->service_type = service_type;

    util::IPEndPoint local_endpoint;
    local_endpoint.addr = parent->m_impl->GetLocalIPAddress();
    local_endpoint.port = parent->m_impl->GetHttpServer()->GetPort();

    std::string headers = "Callback: <http://"
	+ local_endpoint.ToString()
	+ parent->m_impl->GetGenaCallback() + ">\r\n"
	"NT: upnp:event\r\n"
	"Timeout: Second-7200\r\n";

//    TRACE << "GENA subscribe:\n" << headers;

    util::http::Fetcher fetcher(parent->m_impl->GetHttpClient(),
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
//	TRACE << "Got SID " << sid << "\n";
	m_impl->parent->AddListener(this, sid);
    }
    else
    {
	TRACE << "Can't subscribe, no SID\n";
    }

    return 0;
}

unsigned int ClientConnection::GenaUInt(const std::string& s)
{
    return (unsigned int)strtoul(s.c_str(), NULL, 10);
}

bool ClientConnection::GenaBool(const std::string& s)
{
    return soap::ParseBool(s);
}

void ClientConnection::SetSid(const std::string& sid)
{
    assert(m_impl);
    assert(m_impl->parent);
    m_impl->parent->AddListener(this, sid);
}

unsigned int ClientConnection::SoapAction(const char *action_name,
					  soap::Inbound *result)
{
    soap::Outbound no_params;
    return SoapAction(action_name, no_params, result);
}

class ClientConnection::SoapXMLObserver: public xml::SaxParserObserver
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

unsigned int ClientConnection::SoapXMLObserver::OnBegin(const char *tag)
{
    if (!strchr(tag, ':'))
	m_tag = tag;
    m_value.clear();
    return 0;
}

unsigned int ClientConnection::SoapXMLObserver::OnContent(const char *content)
{
    if (!m_tag.empty())
	m_value += content;
    return 0;
}

unsigned int ClientConnection::SoapXMLObserver::OnEnd(const char *tag)
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

unsigned int ClientConnection::SoapAction(const char *action_name,
					  const soap::Outbound& in,
					  soap::Inbound *result)
{
    std::string headers = "SOAPACTION: \"";
    headers += m_impl->service_type;
    headers += "#";
    headers += action_name;
    headers += "\"\r\nContent-Type: text/xml; charset=\"utf-8\"\r\n";

    std::string body = in.CreateBody(action_name, m_impl->service_type);
	
    util::http::StreamPtr hsp;
    unsigned int rc = util::http::Stream::Create(&hsp, 
						 m_impl->control_url.c_str(),
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

} // namespace upnp
