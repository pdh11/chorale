#include "config.h"

#ifndef WIN32

#include "libutil/worker_thread_pool.h"
#include "libutil/cpus.h"
#include "libutil/http_client.h"
#include "libutil/http_server.h"
#include "libutil/trace.h"
#include "libutil/poll.h"
#include "libutil/hal.h"
#include "libutil/dbus.h"
#include "libreceiver/ssdp.h"
#include "libreceiverd/content_factory.h"
#include "libupnp/server.h"
#include "libupnp/ssdp.h"
#include "liboutput/gstreamer.h"
#include "libupnpd/media_renderer.h"
#include "libupnpd/media_server.h"
#include "libtv/database.h"
#include "libtv/epg.h"
#include "libtv/web_epg.h"
#include "libtv/recording.h"
#include "libtv/dvb_service.h"
#include "libtv/dvb.h"
#include "libmediadb/schema.h"
#include "libdbmerge/db.h"
#include "cd.h"
#include "database.h"
#include "ip_filter.h"
#include "nfs.h"
#include "web.h"
#include <stdarg.h>
#include <getopt.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/syslog.h>

#if defined(HAVE_LINUX_DVB_DMX_H) && defined(HAVE_LINUX_DVB_FRONTEND_H)
#define HAVE_DVB 1
#endif

#define DEFAULT_DB_FILE LOCALSTATEDIR "/chorale/db.xml"
#define DEFAULT_TIMER_DB_FILE LOCALSTATEDIR "/chorale/timer-db.xml"
#define DEFAULT_WEB_DIR CHORALE_DATADIR "/chorale"
#define DEFAULT_ARF_FILE CHORALE_DATADIR "/chorale/receiver.arf"
/** This file probably already exists for non-Chorale reasons, so its path
 * isn't relative to Chorale's install directory.
 */
#define DEFAULT_CHANNELS_CONF "/etc/channels.conf"

/** This port is not officially allocated. Probably it was someone's birthday.
 */
#define DEFAULT_WEB_PORT 12078
#define DEFAULT_WEB_PORT_S "12078"

namespace choraled {

static void Usage(FILE *f)
{
    fprintf(f,
	 "Usage: choraled [options] [<root-directory> [<flac-root-directory>]]\n"
"Serve local media resources (files, inputs, outputs), to network clients.\n"
"\n"
"The options are:\n"
" -d, --no-daemon    Don't daemonise\n"
" -f, --dbfile=FILE  Database file (default=" DEFAULT_DB_FILE ")\n"
" -t, --threads=N    Use max N threads to scan files (default 32)\n"
" -r, --no-receiver  Don't become a Rio Receiver server\n"
"     --arf=FILE       Boot from ARF (default=" DEFAULT_ARF_FILE ")\n"
"     --nfs=SERVER     Boot using real NFS net-boot on SERVER\n"
" -w, --web=DIR      Web server root dir (default=" DEFAULT_WEB_DIR ")\n"
" -p, --port=PORT      Web server port (see below for default)\n"
#ifdef HAVE_GSTREAMER
" -a, --no-audio     Don't become a UPnP MediaRenderer server\n"
#endif
" -m, --no-mserver   Don't become a UPnP MediaServer server\n"
#ifdef HAVE_CD
" -o, --no-optical   Don't serve optical (CD) drives for network ripping\n"
#endif
//"     --upnp2receiver Gateway existing UPnP server to Receiver clients\n"
//"     --receiver2upnp Gateway existing Receiver server to UPnP clients\n"
#ifdef HAVE_DVB
" -b, --no-broadcast Don't serve DVB channels\n"
" -c, --channels=FILE  Use FILE as DVB channel list (default=" DEFAULT_CHANNELS_CONF ")\n"
"     --timer-db=FILE  Database of timer recordings (default=" DEFAULT_TIMER_DB_FILE ")\n"
#endif
" -h, --help         These hastily-scratched notes\n"
	    "\n"
"  Omitting <root-directory> means no local files are served, so -f and -t are\n"
"then ignored. If -b is also in effect, or if DVB support is not present, -m\n"
"and -r are also implicitly activated (as there would be nothing to serve).\n"
"  Omitting <root-directory> also disables TV and radio timer recording, as\n"
"there would be nowhere to record them to.\n"
"  The options --nfs and --arf are mutually exclusive; --arf is the default.\n"
"They do not affect Empeg-Car Receiver Edition, which always boots locally.\n"
"  By default, the port used is " DEFAULT_WEB_PORT_S ", as used by 'traditional' Receiver\n"
"servers, unless -r is in effect, in which case the default port is 0, meaning\n"
"pick an arbitrary unused port. In either case, the default can be overridden\n"
"using -p.\n"
#ifdef HAVE_LIBWRAP
"\n"
"  Use /etc/hosts.allow and /etc/hosts.deny to confer or deny access to\n"
"choraled, using the daemon-names 'choraled' or, for read-only access,\n"
"'choraled.ro'. See 'man hosts_access'.\n"
#endif
"\n"
"  Send SIGHUP to force a media re-scan (not needed on systems with 'inotify'\n"
"support).\n"
"\n"
"From " PACKAGE_STRING " (" PACKAGE_WEBSITE ") built on " __DATE__ ".\n"
	);
}

static volatile bool s_exiting = false;
static volatile bool s_rescan = false;
static util::PollerInterface *s_poller = NULL;

static void SignalHandler(int sig)
{
    if (sig == SIGTERM || sig == SIGINT)
	s_exiting = true;
    else if (sig == SIGHUP)
	s_rescan = true;
    else
	return;

    if (s_poller)
	s_poller->Wake();
}

/** Have we become a daemon (yet)? */
static bool s_daemonised = false;

/** Deep Unix fu straight out of W Richard Stevens */
static void daemonise()
{
    if (fork() != 0)
	exit(0); // parent terminates

    setsid(); // become session leader

    signal(SIGHUP, SIG_IGN);

    if (fork() != 0)
	exit(0); // parent terminates

    int rc = chdir("/");
    if (rc<0)
	TRACE << "Can't chdir\n";

    umask(0);

    for (int i=0; i<64; ++i)
	close(i);

    openlog(PACKAGE_NAME, LOG_PID, LOG_USER);

    int fd = open("/dev/null",O_RDONLY);
    if (fd != 0)
	dup2(fd,0);
    if (fd != 1)
	dup2(fd,1);
    if (fd != 2)
	dup2(fd,2);
    if (fd > 2)
	close(fd);

    s_daemonised = true;
}

static void complain(int loglevel, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    if (s_daemonised)
	vsyslog(loglevel, format, args);
    else
	vfprintf(stderr, format, args);
    
    va_end(args);
}

int Main(int argc, char *argv[])
{
    static const struct option options[] =
    {
	{ "help",  no_argument, NULL, 'h' },
	{ "no-audio", no_argument, NULL, 'a' },
	{ "no-daemon", no_argument, NULL, 'd' },
	{ "no-mserver", no_argument, NULL, 'm' },
	{ "no-optical", no_argument, NULL, 'o' },
	{ "no-receiver", no_argument, NULL, 'r' },
	{ "no-broadcast", no_argument, NULL, 'b' },
	{ "web", required_argument, NULL, 'w' },
	{ "nfs", required_argument, NULL, 1 },
	{ "arf", required_argument, NULL, 2 },
	{ "dbfile", required_argument, NULL, 'f' },
	{ "threads", required_argument, NULL, 't' },
	{ "channels", required_argument, NULL, 'c' },
	{ "timer-db", required_argument, NULL, 3 },
	{ NULL, 0, NULL, 0 }
    };

    unsigned int nthreads = 0;
    bool do_daemon = true;
# ifdef HAVE_GSTREAMER
    bool do_audio = true;
# else
    bool do_audio = false;
# endif
    bool do_mserver = true;
    bool do_receiver = true;
#ifdef HAVE_DVB
    bool do_dvb = true;
#else
    bool do_dvb = false;
#endif
    bool do_cd = true;
    bool do_db = true;
    const char *dbfile = DEFAULT_DB_FILE;
    const char *webroot = DEFAULT_WEB_DIR;
    const char *software_server = NULL; // Null string means use ARF
    const char *arf = DEFAULT_ARF_FILE;
    const char *channelsconf = DEFAULT_CHANNELS_CONF;
    const char *timer_db = DEFAULT_TIMER_DB_FILE;
    unsigned short port = DEFAULT_WEB_PORT;
    bool explicit_port = false;

    int option_index;
    int option;
    while ((option = getopt_long(argc, argv, "abrdhmot:f:l:c:p:", options,
				 &option_index))
	   != -1)
    {
	switch (option)
	{
	case 'h':
	    Usage(stdout);
	    return 0;
	case 'd':
	    do_daemon = false;
	    break;
	case 't':
	    nthreads = (unsigned int)strtoul(optarg, NULL, 10);
	    break;
	case 'p':
	    port = (unsigned short)strtoul(optarg, NULL, 10);
	    explicit_port = true;
	    break;
	case 'f':
	    dbfile = optarg;
	    break;
	case 'w':
	    webroot = optarg;
	    break;
	case 'c':
	    channelsconf = optarg;
	    break;
	case 'a':
	    do_audio = false;
	    break;
	case 'm':
	    do_mserver = false;
	    break;
	case 'o':
	    do_cd = false;
	    break;
	case 'r':
	    do_receiver = false;
	    break;
	case 'b':
	    do_dvb = false;
	    break;
	case 1:
	    software_server = optarg;
	    break;
	case 2:
	    arf = optarg;
	    break;
	case 3:
	    timer_db = optarg;
	    break;
	default:
	    Usage(stderr);
	    return 1;
	}
    }

    if (argc > optind+2)
    {
	Usage(stderr);
	return 1;
    }

    if (argc <= optind)
    {
	// No music directory given -- disable all file ops
	do_db = false;
    }

    if (!do_db && !do_dvb)
    {
	// Nothing to serve
	do_mserver = false;
	do_receiver = false;
    }

    if (!do_audio && !do_receiver && !do_mserver && !do_cd)
    {
	complain(LOG_WARNING, "No protocols to serve; exiting\n");
	return 0;
    }

    if (!explicit_port)
    {
	if (do_receiver)
	    port = DEFAULT_WEB_PORT;
	else
	    port = 0;
    }

    if (do_daemon)
	daemonise();

    output::gstreamer::URLPlayer player;
    if (do_audio)
    {
	// Start this off early, as it fork()s internally
	player.Init();
    }

    if (!nthreads)
	nthreads = 32;

    const char *mediaroot = NULL;
    const char *flacroot  = NULL;
    if (do_db)
    {
	mediaroot = argv[optind];
	flacroot  = argv[optind+1];
	if (!flacroot)
	    flacroot = "";
    }

    util::WorkerThreadPool wtp_low(util::WorkerThreadPool::LOW, nthreads);
    util::WorkerThreadPool wtp_normal(util::WorkerThreadPool::NORMAL);
    util::WorkerThreadPool wtp_real_time(util::WorkerThreadPool::HIGH);

    Database db;
#ifdef HAVE_DB
    if (do_db)
	db.Init(mediaroot, flacroot, &wtp_low, dbfile);
#endif

    util::Poller poller;
    util::TaskPoller tp_real_time(&poller, &wtp_real_time);

    tv::dvb::Frontend dvb_frontend;
    if (do_dvb)
    {
	unsigned rc = dvb_frontend.Open(0,0);
	if (rc != 0)
	{
	    complain(LOG_WARNING, "Can't open DVB frontend: %u\n", rc);
	    do_dvb = false;
	}
    }

    tv::dvb::Channels dvbc;
    if (do_dvb)
    {
	unsigned rc = dvbc.Load(channelsconf);
	if (rc != 0)
	{
	    complain(LOG_WARNING, "Can't load DVB channels from '%s': %u\n",
		     channelsconf, rc);
	    do_dvb = false;
	}
    }

    if (do_dvb)
    {
	if (dvbc.Count() == 0)
	{
	    complain(LOG_WARNING, "No DVB channels found\n");
	    do_dvb = false;
	}
    }

    if (do_dvb)
    {
	dvb_frontend.Tune(*dvbc.begin());

	int tries = 0;
	bool tuned;
	do {
	    dvb_frontend.GetState(NULL, NULL, &tuned);
	    if (tuned)
		break;
	    sleep(1);
	    ++tries;
	} while (tries < 5);

	if (!tuned)
	{
	    complain(LOG_WARNING, "Can't tune DVB frontend\n");
	    do_dvb = false;
	}
    }

    if (!do_db && !do_dvb)
    {
	// Nothing to serve
	do_mserver = false;
	do_receiver = false;
    }

    if (!do_audio && !do_receiver && !do_mserver && !do_cd)
    {
	complain(LOG_WARNING, "No protocols to serve; exiting\n");
	return 0;
    }

    tv::dvb::EPG epg;

    // Reading the device is done on a RT thread, writing to disk on a
    // normal thread.
    tv::dvb::Service dvb_service(&dvb_frontend, 
				 &dvbc, 
				 do_db ? (std::string(mediaroot) + "/Recordings")
				 : std::string(),
				 &tp_real_time, &wtp_normal);

    tv::Database tvdb(&dvb_service, &dvbc);

    if (do_dvb)
	epg.Init(&dvb_frontend, &dvbc, &poller, timer_db, &dvb_service);

    db::merge::Database mergedb;
    if (do_db)
	mergedb.AddDatabase(db.Get());
    if (do_dvb)
	mergedb.AddDatabase(&tvdb);

#ifdef HAVE_LIBWRAP
    choraled::IPFilter ipf;
    util::IPFilter *pipf = &ipf;
    complain(LOG_NOTICE, "Using hosts.allow/hosts.deny filtering\n");
#else
    util::IPFilter *pipf = NULL;
#endif

    util::http::Server ws(&poller, &wtp_normal, pipf);

    receiverd::ContentFactory rcf(&mergedb);
    util::http::FileContentFactory fcf(std::string(webroot)+"/upnp",
				       "/upnp");
    util::http::FileContentFactory fcf2(std::string(webroot)+"/layout",
					"/layout");

    if (do_receiver || do_mserver)
    {
	ws.AddContentFactory("/tags", &rcf);
	ws.AddContentFactory("/query", &rcf);
	ws.AddContentFactory("/content", &rcf);
	ws.AddContentFactory("/results", &rcf);
	ws.AddContentFactory("/list", &rcf);
	ws.AddContentFactory("/layout", &fcf2);
    }
    ws.AddContentFactory("/upnp", &fcf);

    tv::WebEPG wepg;
    if (do_dvb)
    {
	wepg.Init(&ws, epg.GetDatabase(), &dvbc, &dvb_service);
    }

    RootContentFactory rootcf(db.Get());

    ws.AddContentFactory("/", &rootcf);
    unsigned int rc = ws.Init(port);
    if (rc)
    {
	complain(LOG_ERR, "Can't bind server port %u, exiting\n", port);
	return 1;
    }

    TRACE << "Webserver got port " << ws.GetPort() << "\n";

    NFSService nfs;
    if (do_receiver && !software_server && arf)
    {
	rc = nfs.Init(&poller, pipf, arf);
	if (rc)
	{
	    complain(LOG_WARNING,
		     "Can't open ARF file %s: Rio Receivers will not boot\n",
		     arf);
	    arf = NULL;
	}
    }

    receiver::ssdp::Server pseudossdp(pipf);
    if (do_receiver)
    {
	pseudossdp.Init(&poller);
	pseudossdp.RegisterService(receiver::ssdp::s_uuid_musicserver, 
				   ws.GetPort());
	if (software_server || arf)
	    pseudossdp.RegisterService(receiver::ssdp::s_uuid_softwareserver, 
				       software_server ? (unsigned short)111
				                       : nfs.GetPort(),
				       software_server);
	TRACE << "Receiver service started\n";
    }

    util::hal::Context *halp = NULL;

#ifdef HAVE_HAL
    util::dbus::Connection dbusc(&poller);
    rc = dbusc.Connect(util::dbus::Connection::SYSTEM);
    if (rc)
    {
	TRACE << "Can't connect to D-Bus\n";
    }
    util::hal::Context halc(&dbusc);

    if (!rc)
    {
	rc = halc.Init();
	if (rc == 0)
	{
	    TRACE << "Hal initialised OK\n";
	    halp = &halc;
	}
    }
#endif

    char hostname[256];
    hostname[0] = '\0';
    gethostname(hostname, sizeof(hostname));
    char *dot = strchr(hostname, '.');
    if (dot && dot > hostname)
	*dot = '\0';

    upnp::ssdp::Responder ssdp(&poller, pipf);
    util::http::Client wc;
    upnp::Server upnpserver(&poller, &wc, &ws, &ssdp);

    upnpd::MediaServer mediaserver(&mergedb, &upnpserver);

    if (do_mserver)
    {
	mediaserver.SetFriendlyName((do_db ? std::string(mediaroot)
				     : std::string("Freeview"))
				    + " on " + hostname);
	rc = mediaserver.Init(&upnpserver, do_db ? mediaroot : "DVB");
	if (rc)
	    complain(LOG_WARNING, "Can't start UPnP media server: %u\n", rc);
    }

#ifdef HAVE_GSTREAMER
    upnpd::MediaRenderer mediarenderer(&player);

    if (do_audio)
    {
	mediarenderer.SetFriendlyName(std::string(hostname)
				      + ":/dev/pcmC0D0p");
	rc = mediarenderer.Init(&upnpserver,  "/dev/pcmC0D0p");
	if (rc)
	    complain(LOG_WARNING, "Can't start UPnP media renderer: %u\n", rc);
    }
#endif

#ifdef HAVE_CD
    CDService cds(halp);
    if (do_cd)
	cds.Init(&ws, hostname, &upnpserver);
#endif

    rc = upnpserver.Init();
    if (rc)
	complain(LOG_WARNING, "Can't start UPnP servers: %u\n", rc);

    s_poller = &poller;
    signal(SIGHUP, SignalHandler);
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    while (!s_exiting)
    {
	poller.Poll(util::Poller::INFINITE_MS);
	if (s_rescan && !s_exiting)
	{
	    s_rescan = false;
#ifdef HAVE_DB
	    db.ForceRescan();
#endif
	}
    }

    s_poller = NULL;

    TRACE << "Orderly exit\n";

    return 0;
}

} // namespace choraled

int main(int argc, char *argv[])
{
    return choraled::Main(argc, argv);
}

#endif // !WIN32
