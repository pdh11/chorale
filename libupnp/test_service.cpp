#include "TestService_server.h"
#include "TestService_client.h"
#include "libutil/scheduler.h"
#include "libutil/http_client.h"
#include "libutil/http_server.h"
#include "libutil/trace.h"
#include "libutil/worker_thread_pool.h"
#include "server.h"
#include "ssdp.h"
#include <boost/format.hpp>
#if HAVE_WINDOWS_H
#include <windows.h>
#endif

const char s_test_device_type[]  = "urn:chorale-sf-net:device:TestDevice:1";
const char s_test_service_id[]   = "urn:chorale-sf-net:serviceId:TestService";
const char s_test_service_type[] = "urn:chorale-sf-net:service:TestService:1";


/* TestServiceImpl */


class TestServiceImpl: public upnp::TestService
{
    unsigned int m_ptang;

public:
    TestServiceImpl() : m_ptang(0) {}

    // Being a TestService
    unsigned int Wurdle(uint32_t fnoo, uint32_t *fnaa);
    unsigned int GetPtang(uint32_t*);
};

unsigned int TestServiceImpl::Wurdle(uint32_t fnoo, uint32_t *fnaa)
{
//    TRACE << "Wurdling\n";
    *fnaa = fnoo + 1;
    m_ptang = fnoo - 1;
//    TRACE << "Firing ptang=" << m_ptang << "\n";
    Fire(&upnp::TestServiceObserver::OnPtang, m_ptang);
    return 0;
}

unsigned int TestServiceImpl::GetPtang(uint32_t *pp)
{
//    TRACE << "Returning ptang " << m_ptang << "\n";
    *pp = m_ptang;
    return 0;
}


/* TestDevice */


class TestDevice: public upnp::Device
{
    TestServiceImpl m_test_service;   
    upnp::TestServiceServer m_test_service_server;

public:
    TestDevice()
	: upnp::Device(s_test_device_type),
	  m_test_service_server(this, s_test_service_id, s_test_service_type,
				"/upnp/TestService.xml", &m_test_service)
	{
	}
};


/* The testing */


struct PtangObserver: public upnp::TestServiceObserver
{
    unsigned int ptang;

    // Being a TSO
    void OnPtang(uint32_t p) { ptang = p; }
};

void DoTest(upnp::TestService *service)
{
    PtangObserver pto;
    pto.ptang = 0;

    service->AddObserver(&pto);
    
    uint32_t fnaa;
    unsigned rc = service->Wurdle(6, &fnaa);
    assert(rc == 0);
    assert(fnaa == 7);

    int tries = 0;
    for (;;)
    {
	if (pto.ptang == 5) // set via eventing
	    break;

	assert(++tries < 30);
#ifdef WIN32
	Sleep(1000);
#else
	sleep(1);
#endif
    }

    /** Relies on QueryStateVariable, which is deprecated */
//    uint32_t ptang;
//    rc = service->GetPtang(&ptang);
//    assert(rc == 0);
//    assert(ptang == 5);

    assert(pto.ptang == 5); // set via eventing

    service->RemoveObserver(&pto);
}

class AsyncTest
{
    upnp::DeviceClient *m_client;
    upnp::TestServiceClientAsync m_tsca;
    bool m_done;

    unsigned int DoWurdle(unsigned int rc, const uint32_t *fnaa)
    {
	TRACE << "In DoWurdle\n";
	assert(rc == 0);
	assert(*fnaa == 17);
	m_done = true;
	return rc;
    }
public:
    AsyncTest(upnp::DeviceClient *client)
	: m_client(client),
	  m_tsca(client, s_test_service_id),
	  m_done(false)
	{}

    ~AsyncTest() { assert(m_done); }

    bool IsDone() const { return m_done; }

    void Run(const std::string& descurl, const std::string& udn)
    {
	TRACE << "In Run\n";
	unsigned int rc = m_client->Init(descurl, udn,
				       util::Bind1<unsigned int, AsyncTest,
				                   &AsyncTest::OnInit>(this));
	assert(rc == 0);

    }

    unsigned int OnInit(unsigned int rc)
    {
	TRACE << "In OnInit\n";
	assert(rc == 0);

	TRACE << "Calling tsca.Init\n";
	rc = m_tsca.Init(util::Bind1<unsigned int, AsyncTest,
			 &AsyncTest::OnInit2>(this));
	TRACE << "tsca.Init returns\n";
	assert(rc == 0);
	return rc;
    }

    unsigned int OnInit2(unsigned int rc)
    {
	TRACE << "In OnInit2\n";
	assert(rc == 0);

	rc = m_tsca.Wurdle(16, 
			   util::Bind2<unsigned int, const uint32_t*, AsyncTest,
			               &AsyncTest::DoWurdle>(this));
        assert(rc == 0);
	return rc;
    }
};

int main()
{
    TestServiceImpl tsi;

    DoTest(&tsi);

    // Now via UPnP

    util::WorkerThreadPool wtp(util::WorkerThreadPool::NORMAL);
    util::BackgroundScheduler poller;
    wtp.PushTask(util::SchedulerTask::Create(&poller));
    util::http::Client wc;
    util::http::Server ws(&poller, &wtp);
    upnp::ssdp::Responder ssdp(&poller, NULL);

    ws.Init();
    upnp::Server server(&poller, &wc, &ws, &ssdp);

    TestDevice dev;
    dev.Init(&server, "/");

    unsigned rc = server.Init();
    assert(rc == 0);

    std::string descurl = (boost::format("http://127.0.0.1:%u/upnp/description.xml")
			      % ws.GetPort()).str();

    upnp::DeviceClient client(&wc, &ws, &poller);
    rc = client.Init(descurl, dev.GetUDN());
    assert(rc == 0);

    upnp::TestServiceClient tsc(&client, s_test_service_id);
    rc = tsc.Init();
    assert(rc == 0);

    DoTest(&tsc);

    // Async version

    upnp::DeviceClient client2(&wc, &ws, &poller);
    AsyncTest at(&client2);
    at.Run(descurl, dev.GetUDN());

    int tries = 0;
    for (;;)
    {
	if (at.IsDone())
	    break;
	
	assert(++tries < 30);

#ifdef WIN32
	Sleep(1000);
#else
	sleep(1);
#endif
    }

//    TRACE << "Shutdown begins\n";

    poller.Shutdown();
    wtp.Shutdown();

    return 0;
}
