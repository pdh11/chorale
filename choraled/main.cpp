#include "config.h"
#include "main.h"
#include "libutil/http_client.h"
#include "libutil/http_server.h"
#include "libutil/scheduler.h"
#include "libutil/scheduler_task.h"
#include "libutil/trace.h"
#include "libutil/worker_thread_pool.h"
#include "libutil/bind.h"
#include "liboutput/gstreamer.h"
#include "libreceiver/ssdp.h"
#include "libreceiverd/content_factory.h"
#include "libtv/database.h"
#include "libtv/epg.h"
#include "libtv/web_epg.h"
#include "libtv/dvb_service.h"
#include "libtv/dvb.h"
#include "libupnp/server.h"
#include "libupnp/ssdp.h"
#include "libupnpd/media_renderer.h"
#include "libupnpd/media_server.h"
#include "libdbmerge/db.h"
#include "libdbreceiver/database.h"
#include "cd.h"
#include "database.h"
#include "ip_filter.h"
#include "nfs.h"
#include "web.h"
#include <signal.h>


namespace choraled {

volatile bool s_exiting = false;
volatile bool s_rescan = false;
util::Scheduler *s_poller = NULL;

static void SignalHandler(int sig)
{
    if (sig == SIGTERM || sig == SIGINT)
    {
	s_exiting = true;
	signal(sig, SIG_DFL);
    }
#ifdef SIGHUP
    else if (sig == SIGHUP)
    {
	s_rescan = true;
	signal(sig, SignalHandler);
    }
#endif
    else
	return;

    if (s_poller)
	s_poller->Wake();
}

class ReceiverAssimilator final: public receiver::ssdp::Client::Callback
{
    db::receiver::Database m_dbreceiver;
    db::merge::Database *m_dbmerge;
    bool m_found_one;

public:
    ReceiverAssimilator(util::http::Client *http_client,
			db::merge::Database *dbmerge)
	: m_dbreceiver(http_client),
	  m_dbmerge(dbmerge),
	  m_found_one(false)
    {
    }

    void OnService(const util::IPEndPoint&) override;
};

void ReceiverAssimilator::OnService(const util::IPEndPoint& ipe)
{
    if (m_found_one)
	return;

    unsigned int rc = m_dbreceiver.Init(ipe);
    if (rc != 0)
    {
	TRACE << "Can't open dialogue with Receiver server at "
	      << ipe.ToString() << "\n";
    }
    else
    {
	TRACE << "Assimilating Receiver server at " << ipe.ToString()
	      << "\n";
	m_dbmerge->AddDatabase(&m_dbreceiver);
	m_found_one = true;
    }
}

int Main(const Settings *settings, Complaints *complaints)
{
#if HAVE_GSTREAMER
    output::gstreamer::URLPlayer player;
    if (settings->flags & AUDIO)
    {
	// Start this off early, as it fork()s internally
	player.Init();
    }
#endif

    util::WorkerThreadPool wtp_low(util::WorkerThreadPool::LOW, 
				   settings->nthreads ? settings->nthreads
				                      : 32);
    util::WorkerThreadPool wtp_normal(util::WorkerThreadPool::NORMAL);
    util::WorkerThreadPool wtp_real_time(util::WorkerThreadPool::HIGH);

    
    db::merge::Database mergedb;
    util::http::Client wc;
    util::BackgroundScheduler poller;

#if HAVE_TAGLIB
    LocalDatabase localdb(&wc);
    if (settings->flags & LOCAL_DB)
    {
	localdb.Init(settings->media_root, settings->flac_root, &poller,
		     &wtp_low, settings->database_file);
	mergedb.AddDatabase(localdb.Get());
    }
#endif

    util::BackgroundScheduler tp_real_time;
    wtp_real_time.PushTask(util::SchedulerTask::Create(&tp_real_time));

#if HAVE_LIBWRAP
    choraled::IPFilter ipf;
    util::IPFilter *pipf = &ipf;
    complaints->Complain(LOG_NOTICE, 
			 "Using hosts.allow/hosts.deny filtering\n");
#else
    util::IPFilter *pipf = NULL;
#endif

    util::http::Server ws(&poller, &wtp_normal, pipf);

#if HAVE_DVB
    tv::dvb::Frontend dvb_frontend;
    tv::dvb::Channels dvb_channels;
    tv::dvb::EPG epg;

    std::string recordings_dir =
	settings->media_root
	    ? std::string(settings->media_root) + "/Recordings"
	    : std::string();

    // Reading the device is done on a RT thread, writing to disk on a
    // normal thread.
    tv::dvb::Service dvb_service(&dvb_frontend, &dvb_channels, recordings_dir,
				 &tp_real_time, &wtp_normal);
    tv::Database tvdb(&dvb_service, &dvb_channels);
    tv::WebEPG wepg;

    if (settings->flags & DVB)
    {
	unsigned rc = dvb_frontend.Open(0,0);
	if (rc != 0)
	{
	    complaints->Complain(LOG_WARNING, 
				 "Can't open DVB frontend: %u\n", rc);
	}
	else
	{
	    rc = dvb_channels.Load(settings->dvb_channels_file);
	    if (rc != 0 || dvb_channels.Count() == 0)
	    {
		complaints->Complain(LOG_WARNING,
				     "Can't load DVB channels from '%s': %u\n",
				     settings->dvb_channels_file, rc);
	    }
	    else
	    {
		epg.Init(&dvb_frontend, &dvb_channels, &poller, 
			 settings->timer_database_file, &dvb_service);

		tvdb.Init();
		mergedb.AddDatabase(&tvdb);
		wepg.Init(&ws, epg.GetDatabase(), &dvb_channels, &dvb_service);
	    }
	}
    }
#endif

    receiverd::ContentFactory rcf(&mergedb);
    util::http::FileContentFactory fcf(
	std::string(settings->web_root)+"/upnp", "/upnp");
    util::http::FileContentFactory fcf2(
	std::string(settings->web_root)+"/layout", "/layout");

    if ((settings->flags & (RECEIVER | MEDIA_SERVER)) != 0)
    {
	ws.AddContentFactory("/tags", &rcf);
	ws.AddContentFactory("/query", &rcf);
	ws.AddContentFactory("/content", &rcf);
	ws.AddContentFactory("/results", &rcf);
	ws.AddContentFactory("/list", &rcf);
	ws.AddContentFactory("/layout", &fcf2);
    }
    ws.AddContentFactory("/upnp", &fcf);

    RootContentFactory rootcf(&mergedb);

    ws.AddContentFactory("/", &rootcf);
    unsigned int rc = ws.Init(settings->web_port);
    if (rc)
    {
	complaints->Complain(LOG_ERR, "Can't bind server port %u, exiting\n",
			     settings->web_port);
	return 1;
    }

    TRACE << "Webserver got port " << ws.GetPort() << "\n";

    NFSService nfs;
    if ((settings->flags & RECEIVER)
	&& settings->receiver_software_server == NULL
	&& settings->receiver_arf_file != NULL)
    {
	rc = nfs.Init(&poller, pipf, settings->receiver_arf_file);
	if (rc)
	{
	    complaints->Complain(LOG_WARNING,
		     "Can't open ARF file %s: Rio Receivers will not boot\n",
				 settings->receiver_arf_file);
	}
    }

    receiver::ssdp::Server pseudossdp(pipf);
    if (settings->flags & RECEIVER)
    {
	pseudossdp.Init(&poller);
	pseudossdp.RegisterService(receiver::ssdp::s_uuid_musicserver, 
				   ws.GetPort());
	if (settings->receiver_software_server
	    || settings->receiver_arf_file)
	    pseudossdp.RegisterService(receiver::ssdp::s_uuid_softwareserver, 
				       settings->receiver_software_server
				           ? (unsigned short)111
  				           : nfs.GetPort(),
				       settings->receiver_software_server);
	TRACE << "Receiver service started\n";
    }

    receiver::ssdp::Client pseudossdp_client;
    ReceiverAssimilator ras(&wc, &mergedb);
    if (settings->flags & ASSIMILATE_RECEIVER)
    {
	pseudossdp_client.Init(&poller, receiver::ssdp::s_uuid_musicserver,
			       &ras);
    }

    char hostname[256];
    hostname[0] = '\0';
    gethostname(hostname, sizeof(hostname));
    char *dot = strchr(hostname, '.');
    if (dot && dot > hostname)
	*dot = '\0';

    upnp::ssdp::Responder ssdp(&poller, pipf);
    upnp::Server upnpserver(&poller, &wc, &ws, &ssdp);

    upnpd::MediaServer mediaserver(&mergedb, &upnpserver);

    if (settings->flags & MEDIA_SERVER)
    {
	mediaserver.SetFriendlyName(std::string(PACKAGE_NAME)
				    + " on " + hostname);
	rc = mediaserver.Init(&upnpserver, 
			      settings->media_root ? settings->media_root : "DVB");
	if (rc)
	    complaints->Complain(LOG_WARNING, 
				 "Can't start UPnP media server: %u\n", rc);
    }

#if HAVE_GSTREAMER
    upnpd::MediaRenderer mediarenderer(&player);

    if (settings->flags & AUDIO)
    {
	mediarenderer.SetFriendlyName(std::string(hostname)
				      + ":/dev/pcmC0D0p");
	rc = mediarenderer.Init(&upnpserver,  "/dev/pcmC0D0p");
	if (rc)
	    complaints->Complain(LOG_WARNING,
				 "Can't start UPnP media renderer: %u\n", rc);
    }
#endif

#if HAVE_CD
    CDService cds;
    if (settings->flags & CD)
	cds.Init(&ws, hostname, &upnpserver);
#endif

    rc = upnpserver.Init();
    if (rc)
	complaints->Complain(LOG_WARNING, 
			     "Can't start UPnP servers: %u\n", rc);

    s_poller = &poller;

#ifdef SIGHUP
    signal(SIGHUP, choraled::SignalHandler);
#endif
    signal(SIGINT, choraled::SignalHandler);
    signal(SIGTERM, choraled::SignalHandler);

    while (!s_exiting)
    {
	poller.Poll(util::BackgroundScheduler::INFINITE_MS);
	if (s_rescan && !s_exiting)
	{
	    s_rescan = false;
#if HAVE_TAGLIB
	    if (settings->flags & LOCAL_DB)
		localdb.ForceRescan();
#endif
	}
    }

    s_poller = NULL;

    TRACE << "Orderly exit\n";

    return 0;
}

} // namespace choraled
