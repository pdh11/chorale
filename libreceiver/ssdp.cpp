#include "ssdp.h"
#include "libutil/poll.h"
#include "libutil/socket.h"
#include "libutil/trace.h"
#include <boost/thread/mutex.hpp>
#include <map>
#include <sstream>

namespace receiver {

namespace ssdp {

const char *s_uuid_softwareserver = 
    "upnp:uuid:1D274DB1-F053-11d3-BF72-0050DA689B2F";

const char *s_uuid_musicserver = 
    "upnp:uuid:1D274DB0-F053-11d3-BF72-0050DA689B2F";


        /* Server */


class Server::Impl: public util::Pollable
{
    util::DatagramSocket m_socket;

    struct Service 
    {
	unsigned short port;
	const char *host; // Null means "this host"
    };

    typedef std::map<std::string, Service> services_t;

    boost::mutex m_mutex;
    services_t m_services;

public:
    Impl();

    unsigned Init(util::PollerInterface *poller);
    void RegisterService(const char *uuid, unsigned short service_port,
			 const char *service_host);
    
    // being a Pollable
    unsigned OnActivity();
};

Server::Impl::Impl()
{
}

unsigned Server::Impl::Init(util::PollerInterface *poller)
{
    util::IPEndPoint ep = { util::IPAddress::ANY, 21075 };
    unsigned rc = m_socket.Bind(ep);
    if (rc == 0)
	m_socket.SetNonBlocking(true);
    if (rc == 0)
	poller->AddHandle(m_socket.GetPollHandle(), this, util::Poller::IN);

    TRACE << "Server init returned " << rc << "\n";

    return rc;
}

void Server::Impl::RegisterService(const char *uuid,
				   unsigned short service_port,
				   const char *service_host)
{
    boost::mutex::scoped_lock lock(m_mutex);
    Service svc;
    svc.port = service_port;
    svc.host = service_host;
    m_services[uuid] = svc;
}

unsigned Server::Impl::OnActivity()
{
    char buffer[1500];
    
    size_t nread;
    util::IPEndPoint client;
    util::IPAddress myip;
    unsigned rc = m_socket.Read(buffer, sizeof(buffer)-1, &nread, &client,
				&myip);
    TRACE << "Server read returned " << rc << "\n";
    if (rc != 0)
	return rc;
    if (nread == 0)
	return 0;

    buffer[nread] = 0;

    char *lf = strchr(buffer, '\n');
    if (!lf)
	return 0;
    
    *lf = 0;
    
    TRACE << "Server: '''" << buffer << "'''\n";

    std::string reply;
    {
	boost::mutex::scoped_lock lock(m_mutex);
	services_t::const_iterator i = m_services.find(std::string(buffer));

	if (i == m_services.end())
	    return 0;

	std::string host = i->second.host ? i->second.host : myip.ToString();
	std::ostringstream os;
	os << "http://" << host << ":" << i->second.port
	   << "/descriptor.xml\n";
	reply = os.str();
    }

    TRACE << "Reply: " << reply;

    return m_socket.Write(reply.c_str(), reply.length(), client);
}

Server::Server()
    : m_impl(new Impl)
{
}

Server::~Server()
{
    delete m_impl;
}

unsigned Server::Init(util::PollerInterface *p)
{
    return m_impl->Init(p);
}

void Server::RegisterService(const char *uuid, unsigned short service_port,
			     const char *service_host)
{
    m_impl->RegisterService(uuid, service_port, service_host);
}


        /* Client */


class Client::Impl: public util::Pollable
{
    util::DatagramSocket m_socket;
    Client::Callback *m_callback;

public:
    unsigned Init(util::PollerInterface*, const char *uuid, Callback*);

    // being a Pollable
    unsigned OnActivity();
};

unsigned Client::Impl::Init(util::PollerInterface *poller,
			    const char *uuid, Callback *cb)
{
    unsigned rc = m_socket.EnableBroadcast(true);
    if (rc == 0)
	m_socket.SetNonBlocking(true);
    if (rc == 0)
	poller->AddHandle(m_socket.GetPollHandle(), this, util::Poller::IN);

    m_callback = cb;

    util::IPEndPoint ep = { util::IPAddress::ALL, 21075 };
    std::string request = uuid;
    request += "\n";
    return m_socket.Write(request.c_str(), request.length(), ep);
}

unsigned Client::Impl::OnActivity()
{
    char buffer[1500];   
    size_t nread;
    util::IPEndPoint client;
    util::IPAddress myip;
    unsigned rc = m_socket.Read(buffer, sizeof(buffer)-1, &nread, &client,
				&myip);
    if (rc != 0)
	return rc;
    if (nread == 0)
	return 0;

    buffer[nread] = 0;

//    TRACE << "ssdp received " <<  buffer << "\n";

    int a,b,c,d,e;
    if (sscanf(buffer, "http://%d.%d.%d.%d:%d/", &a, &b, &c, &d, &e) == 5)
    {
	util::IPEndPoint ep;
	ep.addr = util::IPAddress::FromDottedQuad(a,b,c,d);
	ep.port = e;
	m_callback->OnService(ep);
    }
    else
    {
	TRACE << "ssdp didn't like it\n";
    }

    return 0;
}

Client::Client()
    : m_impl(new Impl)
{
}

Client::~Client()
{
    delete m_impl;
}

unsigned Client::Init(util::PollerInterface *p, const char *uuid,
		      Callback *cb)
{
    return m_impl->Init(p, uuid, cb);
}

}; // namespace ssdp

}; // namespace receiver
