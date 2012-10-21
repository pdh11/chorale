#include "context.h"
#include "libutil/trace.h"
#include <sys/stat.h>
#include <sys/types.h>

const char data_root[] = "/var/mobile/Applications/Chorale";

Context::Context()
    : m_media_renderer(&m_player),
      m_thread_pool(util::WorkerThreadPool::NORMAL, 4),
      m_http_server(&m_fg_scheduler, &m_thread_pool, NULL),
      m_ssdp_responder(&m_fg_scheduler, NULL),
      m_upnp_server(&m_fg_scheduler, &m_http_client, &m_http_server,
		    &m_ssdp_responder),
      m_dav_server(data_root, "/dav")
{
    TRACE << "Context constructed\n";
}

Context::~Context()
{
}

unsigned int Context::Init()
{
    TRACE << "Init\n";
    m_media_renderer.SetFriendlyName("Iphone");
    TRACE << "Init2\n";
    unsigned int rc = m_media_renderer.Init(&m_upnp_server, "Iphone Audio");
    TRACE << "Init3\n";
    if (!rc)
	rc = m_upnp_server.Init();
    TRACE << "Init4\n";
    mkdir(data_root, 0666);
    TRACE << "Init5\n";
    m_http_server.AddContentFactory("/dav", &m_dav_server);
    TRACE << "Init6\n";
    m_http_server.Init(12078);
    TRACE << "Init7\n";
    return rc;
}

void Srsly(const char *s)
{
    TRACE << s;
}
