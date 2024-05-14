#include "ssdp.h"
#include "config.h"
#include "libutil/scheduler.h"
#include "libutil/bind.h"
#include "libutil/socket.h"
#include "libutil/trace.h"
#include "libutil/ip_filter.h"
#include "libutil/ip_config.h"
#include "libutil/task.h"
#include <boost/format.hpp>
#include <map>
#include <mutex>
#include <sstream>
#include <string.h>
#include <stdio.h>

// We need IFF_BROADCAST
#if HAVE_NET_IF_H
# include <netinet/in.h>
# include <net/if.h>
#elif HAVE_WS2TCPIP_H
# include <ws2tcpip.h>
#endif

#undef IN
#undef OUT

namespace receiver {

namespace ssdp {

const char s_uuid_softwareserver[] = 
    "upnp:uuid:1D274DB1-F053-11d3-BF72-0050DA689B2F";

const char s_uuid_musicserver[] = 
    "upnp:uuid:1D274DB0-F053-11d3-BF72-0050DA689B2F";

/** Receivers expect their pseudo-SSDP server on port 21075. This port is not
 * officially allocated, so theoretically could already be in use by something
 * else. @todo Check for this.
 *
 * 21075 was originally chosen as it's Mike's birthday.
 */
enum { PORT = 21075 };


        /* Server */


class Server::Task: public util::Task
{
    util::IPFilter *m_filter;
    util::DatagramSocket m_socket;

    struct Service 
    {
	unsigned short port;
	const char *host; // Null means "this host"
    };

    typedef std::map<std::string, Service> services_t;

    std::mutex m_mutex;
    services_t m_services;

public:
    Task(util::IPFilter *ip_filter) : m_filter(ip_filter) {}

    unsigned Init(util::Scheduler *poller);
    void RegisterService(const char *uuid, unsigned short service_port,
			 const char *service_host);
    
    unsigned Run();
};

unsigned Server::Task::Init(util::Scheduler *poller)
{
    util::IPEndPoint ep = { util::IPAddress::ANY, PORT };
    unsigned rc = m_socket.Bind(ep);
    if (rc == 0)
	m_socket.SetNonBlocking(true);
    if (rc == 0)
	poller->WaitForReadable(
	    util::Bind(TaskPtr(this)).To<&Task::Run>(), m_socket.GetHandle(),
	    false);

//    TRACE << "Server init returned " << rc << "\n";

    return rc;
}

void Server::Task::RegisterService(const char *uuid,
				   unsigned short service_port,
				   const char *service_host)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    Service svc;
    svc.port = service_port;
    svc.host = service_host;
    m_services[uuid] = svc;
}

unsigned Server::Task::Run()
{
    char buffer[1500];
    
    size_t nread;
    util::IPEndPoint client;
    util::IPAddress myip;
    unsigned rc = m_socket.Read(buffer, sizeof(buffer)-1, &nread, &client,
				&myip);
//    TRACE << "Server read returned " << rc << "\n";
    if (rc != 0)
	return rc;
    if (nread == 0)
	return 0;

    if (m_filter
	&& m_filter->CheckAccess(client.addr) == util::IPFilter::DENY)
	return 0;

    buffer[nread] = 0;

    char *lf = strchr(buffer, '\n');
    if (!lf)
	return 0;
    
    *lf = 0;
    
//    TRACE << "Server: " << buffer << "\n";

    std::string reply;
    {
	std::lock_guard<std::mutex> lock(m_mutex);
	services_t::const_iterator i = m_services.find(std::string(buffer));

	if (i == m_services.end())
	    return 0;

	std::string host = i->second.host ? i->second.host : myip.ToString();
	std::ostringstream os;
	os << "http://" << host << ":" << i->second.port
	   << "/descriptor.xml\n";
	reply = os.str();
    }

//    TRACE << "Reply: " << reply;

    m_socket.Write(reply.c_str(), reply.length(), client);
    return 0;
}

Server::Server(util::IPFilter *filter)
    : m_task(new Task(filter))
{
}

Server::~Server()
{
//    delete m_task;
}

unsigned Server::Init(util::Scheduler *p)
{
    return m_task->Init(p);
}

void Server::RegisterService(const char *uuid, unsigned short service_port,
			     const char *service_host)
{
    m_task->RegisterService(uuid, service_port, service_host);
}


        /* Client */


class Client::Task: public util::Task
{
    util::DatagramSocket m_socket;
    Client::Callback *m_callback;

public:
    Task() : m_callback(NULL) {}

    unsigned Init(util::Scheduler*, const char *uuid, Callback*);
    unsigned Run();
};

unsigned Client::Task::Init(util::Scheduler *poller,
			    const char *uuid, Callback *cb)
{
    unsigned rc = m_socket.EnableBroadcast(true);
    if (rc == 0)
	m_socket.SetNonBlocking(true);
    if (rc == 0)
	poller->WaitForReadable(
	    util::Bind(TaskPtr(this)).To<&Task::Run>(), m_socket.GetHandle(),
	    false);

    m_callback = cb;
    std::string request = uuid;
    request += "\n";

    util::IPConfig::Interfaces ip_interfaces;
    util::IPConfig::GetInterfaceList(&ip_interfaces);

    for (util::IPConfig::Interfaces::const_iterator i = ip_interfaces.begin();
	 i != ip_interfaces.end();
	 ++i)
    {
	if (i->flags & IFF_BROADCAST)
	{
	    util::IPEndPoint ep;
	    ep.addr = i->broadcast;
	    ep.port = PORT;
	    rc = m_socket.Write(request.c_str(), request.length(), ep);
	    if (rc)
		return rc;
	}
    }
    return 0;
}

unsigned Client::Task::Run()
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

    unsigned int a,b,c,d;
    unsigned short e;
    if (sscanf(buffer, "http://%u.%u.%u.%u:%hu/",
	       &a, &b, &c, &d, &e) == 5)
    {
	util::IPEndPoint ep;
	ep.addr = util::IPAddress::FromDottedQuad((unsigned char)a,
						  (unsigned char)b,
						  (unsigned char)c,
						  (unsigned char)d);
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
    : m_task(new Task)
{
}

Client::~Client()
{
}

unsigned Client::Init(util::Scheduler *p, const char *uuid,
		      Callback *cb)
{
    return m_task->Init(p, uuid, cb);
}

} // namespace ssdp

} // namespace receiver

#ifdef TEST

class TestCallback: public receiver::ssdp::Client::Callback
{
public:
    TestCallback() : m_ok(false) {}

    void OnService(const util::IPEndPoint&)
    {
//	TRACE << "Service on " << ep.ToString() << "\n";
	m_ok = true;
    }

    bool m_ok;
};

int main()
{
    util::DatagramSocket tx;
    util::DatagramSocket rx;
    util::IPEndPoint ep = { util::IPAddress::ANY, 0 };
    rx.Bind(ep);
    ep = rx.GetLocalEndPoint();
//    TRACE << "Got port " << ep.port << "\n";
    tx.EnableBroadcast(true);
    ep.addr = util::IPAddress::ALL;
//    TRACE << "Sending 'test0' to " << ep.addr.ToString() << "\n";
    tx.Write("test0", ep);

    util::IPConfig::Interfaces interface_list;
    
    util::IPConfig::GetInterfaceList(&interface_list);
    
    unsigned int j=0;
    for (util::IPConfig::Interfaces::const_iterator i = interface_list.begin();
	 i != interface_list.end();
	 ++i)
    {
	if (i->flags & IFF_BROADCAST)
	{
	    ep.addr = i->broadcast;
	    std::string message = (boost::format("test%u") % ++j).str();
//	    TRACE << "Sending '" << message << "'to " << ep.addr.ToString() << "\n";
	    tx.Write(message, ep);
	}
	else
	{
//	    TRACE << "Interface " << i->address.ToString()
//		  << " not broadcastable (addr " << i->broadcast.ToString()
//		  << ")\n";
	}
    }
    std::string s;
    util::IPEndPoint wasfrom;
    util::IPAddress wasto;
    for (;;)
    {
	unsigned int rc = rx.WaitForRead(2000);
	if (rc)
	    break;
	rc = rx.Read(&s, &wasfrom, &wasto);
	if (rc != 0)
	{
//	    TRACE << "Read failed, rc=" << rc << "\n";
	}
	assert(rc == 0);
	TRACE << s << " from " << wasfrom.ToString() << " to " << wasto.ToString() << "\n";
    }

    util::BackgroundScheduler poller;
    receiver::ssdp::Server server(NULL);
    unsigned rc = server.Init(&poller);
    assert(rc == 0);
    
    server.RegisterService("test.uuid", 69);

    receiver::ssdp::Client client;
    TestCallback tc;
    rc = client.Init(&poller, "test.uuid", &tc);

    time_t end = ::time(NULL) + 5;

    for(;;)
    {
	long long sec = end - ::time(NULL);
	if (sec < 0 || tc.m_ok)
	    break;
	poller.Poll((unsigned)sec * 1000);
    }

    assert(tc.m_ok);

    return 0;
}

#endif
