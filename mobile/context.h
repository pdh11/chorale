#ifndef MOBILE_CONTEXT_H
#define MOBILE_CONTEXT_H 1

#include "config.h"

#if HAVE_COREFOUNDATION_CFRUNLOOP_H

#include "libuidarwin/scheduler.h"
#include "liboutput/avfoundation.h"
#include "libupnpd/media_renderer.h"
#include "libupnp/server.h"
#include "libutil/http_server.h"
#include "libutil/http_client.h"
#include "libutil/worker_thread_pool.h"
#include "libupnp/ssdp.h"
#include "libdav/server.h"

class Context
{
    ui::darwin::Scheduler m_fg_scheduler;
    output::avfoundation::URLPlayer m_player;
    upnpd::MediaRenderer m_media_renderer;
    util::http::Client m_http_client;
    util::WorkerThreadPool m_thread_pool;
    util::http::Server m_http_server;
    upnp::ssdp::Responder m_ssdp_responder;
    upnp::Server m_upnp_server;
    dav::Server m_dav_server;

public:
    Context();
    ~Context();

    unsigned int Init();
};

void Srsly(const char*);

#endif // HAVE_COREFOUNDATION_CFRUNLOOP_H

#endif
