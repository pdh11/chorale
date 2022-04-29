#include "TestService_server.h"
#include "TestService_client.h"
#include "libutil/bind.h"
#include "libutil/trace.h"
#include "libutil/scheduler.h"
#include "libutil/http_client.h"
#include "libutil/http_server.h"
#include "libutil/worker_thread_pool.h"
#include "ssdp.h"
#include "server.h"
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
    unsigned int Wurdle(
	bool boolin,
	bool *boolout,
            int8_t scharin,
            uint8_t ucharin,
            int8_t *scharout,
            uint8_t *ucharout,
            int16_t shalfin,
            uint16_t uhalfin,
            int16_t *shalfout,
            uint16_t *uhalfout,
            int32_t swordin,
            uint32_t uwordin,
            int32_t *swordout,
            uint32_t *uwordout,
            const std::string& strin,
            std::string *strout,
            const std::string& urlin,
            std::string *urlout,
            Enum enumin,
            Enum *enumout);
    unsigned int GetPtang(uint32_t*);
};

unsigned int TestServiceImpl::Wurdle(
    bool boolin,
    bool *boolout,
            int8_t scharin,
            uint8_t ucharin,
            int8_t *scharout,
            uint8_t *ucharout,
            int16_t shalfin,
            uint16_t uhalfin,
            int16_t *shalfout,
            uint16_t *uhalfout,
            int32_t swordin,
            uint32_t uwordin,
            int32_t *swordout,
            uint32_t *uwordout,
            const std::string& strin,
            std::string *strout,
            const std::string& urlin,
            std::string *urlout,
            Enum enumin,
            Enum *enumout)
{
    TRACE << "Wurdling\n";

    m_ptang = uwordin + 1;

    *boolout = !boolin;
    *scharout = ++scharin;
    *ucharout = --ucharin;
    *shalfout = ++shalfin;
    *uhalfout = --uhalfin;
    *swordout = ++swordin;
    *uwordout = --uwordin;
    *strout = strin + "+";
    *urlout = urlin + "#";
    *enumout = (Enum)(enumin+1);

    TRACE << "Firing ptang=" << m_ptang << "\n";
    Fire(&upnp::TestServiceObserver::OnPtang, m_ptang);
    TRACE << "Fired\n";
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

static void DoTest(upnp::TestService *service)
{
    PtangObserver pto;
    pto.ptang = 0;

    service->AddObserver(&pto);
    
    bool b = false;
    int8_t i1 = 0;
    uint8_t ui1 = 0;
    int16_t i2;
    uint16_t ui2;
    int32_t i4;
    uint32_t ui4;
    std::string str;
    std::string url;
    upnp::TestService::Enum en;
    unsigned rc = service->Wurdle(false, &b, -42, 221, &i1, &ui1,
				  -2001, 50000, &i2, &ui2,
				  -80386, 3u*1000*1000*1000, &i4, &ui4,
				  "foo", &str,
				  "http", &url,
				  upnp::TestService::ENUM_FRINK, &en);
    assert(rc == 0);
    assert(b);
    assert(i1 == -41);
    assert(ui1 == 220);
    assert(i2 == -2000);
    assert(ui2 == 49999);
    assert(i4 == -80385);
    assert(ui4 == 2999999999u);
    assert(str == "foo+");
    assert(url == "http#");
    assert(en == upnp::TestService::ENUM_WAAH);

    int tries = 0;
    for (;;)
    {
	if (pto.ptang) // set via eventing
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

    assert(pto.ptang == 3000000001u); // set via eventing

    service->RemoveObserver(&pto);
}

class AsyncTest: public upnp::TestServiceAsyncObserver
{
    upnp::DeviceClient *m_client;
    upnp::TestServiceClientAsync m_tsca;
    bool m_done;

    void OnWurdleDone(unsigned int rc,
		      bool boolout,
		      int8_t scharout,
		      uint8_t ucharout,
		      int16_t shalfout,
		      uint16_t uhalfout,
		      int32_t swordout,
		      uint32_t uwordout,
		      const std::string& strout,
		      const std::string& urlout,
		      upnp::TestService::Enum enumout)
    {
	TRACE << "In OnWurdleDone\n";
	assert(rc == 0);
	assert(!boolout);
	assert(scharout == 70);
	assert(ucharout == 220);
	assert(shalfout == 748);
	assert(uhalfout == 49999);
	assert(swordout == 68009);
	assert(uwordout == 2999999999u);
	assert(strout == "foonly+");
	assert(urlout == "gopher#");
	assert(enumout == upnp::TestService::ENUM_GIBILISCO);
	m_done = true;
    }
public:
    AsyncTest(upnp::DeviceClient *client)
	: m_client(client),
	  m_tsca(client, s_test_service_id, this),
	  m_done(false)
	{}

    ~AsyncTest() { assert(m_done); }

    bool IsDone() const { return m_done; }

    void Run(const std::string& descurl, const std::string& udn)
    {
	TRACE << "In Run\n";
	unsigned int rc = m_client->Init(descurl, udn,
					 util::Bind(this).To<unsigned int,
				                   &AsyncTest::OnInit>());
	assert(rc == 0);

    }

    unsigned int OnInit(unsigned int rc)
    {
	TRACE << "In OnInit\n";
	assert(rc == 0);

	TRACE << "Calling tsca.Init\n";
	rc = m_tsca.Init(util::Bind(this).To<unsigned int,
			 &AsyncTest::OnInit2>());
	TRACE << "tsca.Init returns\n";
	assert(rc == 0);
	return rc;
    }

    unsigned int OnInit2(unsigned int rc)
    {
	TRACE << "In OnInit2\n";
	assert(rc == 0);

	rc = m_tsca.Wurdle(true, 69, 221,
			   747, 50000,
			   68008, 3u*1000*1000*1000,
			   "foonly", "gopher", upnp::TestService::ENUM_WAAH);
			   
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
