#include "config.h"
#include "discovery.h"
#include "libutil/scheduler.h"
#include "libutil/bind.h"
#include "libutil/socket.h"
#include "libutil/trace.h"
#include "libutil/ip_config.h"
#include "libutil/task.h"
#include <boost/tokenizer.hpp>
#include <set>
#include <string.h>
#if HAVE_NET_IF_H
# include <netinet/in.h>
# include <net/if.h>
#elif HAVE_WS2TCPIP_H
# include <ws2tcpip.h>
#endif

#undef IN

namespace empeg {

enum { DISCOVERY_PORT = 8300 };

class Discovery::Task: public util::Task
{
    util::DatagramSocket m_socket;
    Callback *m_callback;
    std::set<uint32_t> m_already;

    typedef util::CountedPointer<Task> TaskPtr;

public:
    Task() : m_callback(NULL) {}

    unsigned Init(util::Scheduler*, Callback*);

    unsigned Run();

    // being a Timed
    unsigned OnTimer();
};

unsigned Discovery::Task::Init(util::Scheduler *poller, Callback *cb)
{
    unsigned rc = m_socket.EnableBroadcast(true);
    if (rc == 0)
	m_socket.SetNonBlocking(true);
    if (rc == 0)
    {
	util::IPEndPoint ep = { util::IPAddress::ANY, DISCOVERY_PORT };
	rc = m_socket.Bind(ep);
    }
    if (rc == 0)
    {
	poller->WaitForReadable(
	    util::Bind(TaskPtr(this)).To<&Task::Run>(), m_socket.GetHandle(),
	    false);
	poller->Wait(
	    util::Bind(TaskPtr(this)).To<&Task::OnTimer>(), 0, 5000);
    }
    m_callback = cb;

    return rc;
}

unsigned Discovery::Task::OnTimer()
{
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
	    ep.port = DISCOVERY_PORT;
	    m_socket.Write((const void*)"?", 1, ep);
	}
    }
    return 0;
}

unsigned Discovery::Task::Run()
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

//    TRACE << "discovery received from " << client.ToString() << ":\n"
//	  << Hex(buffer, nread);

    if (m_already.find(client.addr.addr) != m_already.end())
	return 0;

    m_already.insert(client.addr.addr);

    std::string content(buffer, buffer+nread);

    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
    boost::char_separator<char> sep("\n");
    tokenizer tok(content, sep);

    std::string name;

    for (tokenizer::iterator i = tok.begin(); i != tok.end(); ++i)
    {
//	TRACE << "tok: " << *i << "\n";

	if (!strncmp(i->c_str(), "name=", 5))
	    name = std::string(i->c_str()+5);
    }

    if (!name.empty())
	m_callback->OnDiscoveredEmpeg(client.addr, name);

    return 0;
}

Discovery::Discovery()
    : m_task(new Task)
{
}

Discovery::~Discovery()
{
}

unsigned Discovery::Init(util::Scheduler *poller, Callback *cb)
{
    return m_task->Init(poller, cb);
}

} // namespace empeg

#ifdef TEST

class DECallback: public empeg::Discovery::Callback
{
public:
    void OnDiscoveredEmpeg(const util::IPAddress& ip, const std::string& name)
    {
	TRACE << "Found Empeg '" << name << "' on " << ip.ToString() << "\n";
    }
};

int main(int, char **)
{
    util::BackgroundScheduler poller;

    empeg::Discovery disc;

    DECallback dec;

    disc.Init(&poller, &dec);

    time_t t = time(NULL);

    while ((time(NULL) - t) < 5)
    {
	poller.Poll(1000);
    }

    return 0;
}

#endif
