#include "TestService_server.h"
#include "TestService_client.h"
#include "libutil/poll_thread.h"
#include "libutil/http_client.h"
#include "libutil/http_server.h"
#include "libutil/trace.h"
#include "server.h"
#include "ssdp.h"
#include <boost/format.hpp>

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
    TRACE << "Wurdling\n";
    *fnaa = fnoo + 1;
    m_ptang = fnoo - 1;
    TRACE << "Firing ptang=" << m_ptang << "\n";
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

#ifdef WIN32
    Sleep(5000);
#else
    sleep(5);
#endif

    /** Relies on QueryStateVariable, which is deprecated */
//    uint32_t ptang;
//    rc = service->GetPtang(&ptang);
//    assert(rc == 0);
//    assert(ptang == 5);

    assert(pto.ptang == 5); // set via eventing
}

int main()
{
    TestServiceImpl tsi;

    DoTest(&tsi);

    // Now via UPnP

    util::PollThread poller;
    util::WorkerThreadPool wtp(util::WorkerThreadPool::NORMAL);
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

    upnp::Client client(&wc, &ws);
    rc = client.Init(descurl, dev.GetUDN());
    assert(rc == 0);

    upnp::TestServiceClient tsc;
    rc = tsc.Init(&client, s_test_service_id);
    assert(rc == 0);

    DoTest(&tsc);

    return 0;
}
