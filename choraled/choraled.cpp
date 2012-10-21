#include "config.h"

#ifndef WIN32

#include "libutil/worker_thread_pool.h"
#include "libutil/cpus.h"
#include "libutil/web_server.h"
#include "libutil/trace.h"
#include "libutil/poll.h"
#include "libutil/hal.h"
#include "libutil/dbus.h"
#include "libimport/file_scanner_thread.h"
#include "libreceiver/ssdp.h"
#include "libreceiverd/content_factory.h"
#include "libupnp/server.h"
#include "liboutput/gstreamer.h"
#include "libupnpd/media_renderer.h"
#include "libupnpd/media_server.h"
#include "libtv/stream_factory.h"
#include "libtv/epg.h"
#include "libtv/web_epg.h"
#include "libtv/recording.h"
#include "libmediadb/schema.h"
#include "cd.h"
#include "database.h"
#include "web.h"
#include <getopt.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/syslog.h>

#if defined(HAVE_LINUX_DVB_DMX_H) && defined(HAVE_LINUX_DVB_FRONTEND_H)
#define HAVE_DVB 1
#endif

#define DEFAULT_DB_FILE LOCALSTATEDIR "/chorale/db.xml"
#define DEFAULT_WEB_DIR CHORALE_DATADIR "/chorale"

static void Usage(FILE *f)
{
    fprintf(f,
	 "Usage: choraled [options] [<root-directory> [<flac-root-directory>]]\n"
"Serve local media resources (files, inputs, outputs), to network clients.\n"
"\n"
"The options are:\n"
" -d, --no-daemon    Don't daemonise\n"
" -f, --dbfile=FILE  Database file (default=" DEFAULT_DB_FILE ")\n"
" -t, --threads=N    Use N threads to scan files (default NCPUS*2)\n"
" -h, --help         These hastily-scratched notes\n"
" -r, --no-receiver  Don't become a Rio Receiver server\n"
" -n, --no-nfs         Don't announce Receiver software on standard NFS\n"
"     --nfs=SERVER     Announce an alternative Receiver software server\n"
" -w, --web=DIR      Web server root dir (default=" DEFAULT_WEB_DIR ")\n"
" -p, --port=PORT      Web server port (default=arbitrary unused port)\n"
//" -i, --interface=IF Network interface to listen on (default=first found)\n"
#ifdef HAVE_UPNP
#ifdef HAVE_GSTREAMER
" -a, --no-audio     Don't become a UPnP MediaRenderer server\n"
#endif
" -m, --no-mserver   Don't become a UPnP MediaServer server\n"
#ifdef HAVE_CD
" -o, --no-optical   Don't serve optical (CD) drives for network ripping\n"
#endif
//"     --upnp2receiver Gateway existing UPnP server to Receiver clients\n"
//"     --receiver2upnp Gateway existing Receiver server to UPnP clients\n"
#endif
#ifdef HAVE_DVB
" -b, --no-broadcast Don't serve DVB channels\n"
" -c, --channels=FILE  Use FILE as DVB channel list (default=/etc/channels.conf)\n"
#endif
	    "\n"
	    "Omitting <root-directory> implies -r, -m, -b and ignores -f, -t.\n"
	    "\n"
	    "Send SIGHUP to force a media re-scan (not needed on systems with 'inotify'\n"
	    "support).\n"
	    "\n"
"From " PACKAGE_STRING " built on " __DATE__ ".\n"
	);
}

static volatile bool s_exiting = false;
static volatile bool s_rescan = false;
static util::PollWaker *s_waker = NULL;

static void SignalHandler(int sig)
{
    if (sig == SIGTERM || sig == SIGINT)
	s_exiting = true;
    else if (sig == SIGHUP)
	s_rescan = true;
    else
	return;

    if (s_waker)
	s_waker->Wake();
}

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
}

int main(int argc, char *argv[])
{
    static const struct option options[] =
    {
	{ "help",  no_argument, NULL, 'h' },
	{ "no-nfs",   no_argument, NULL, 'n' },
	{ "no-audio", no_argument, NULL, 'a' },
	{ "no-daemon", no_argument, NULL, 'd' },
	{ "no-mserver", no_argument, NULL, 'm' },
	{ "no-optical", no_argument, NULL, 'o' },
	{ "no-receiver", no_argument, NULL, 'r' },
	{ "no-broadcast", no_argument, NULL, 'b' },
	{ "web", required_argument, NULL, 'w' },
	{ "nfs", required_argument, NULL, 1 },
	{ "dbfile", required_argument, NULL, 'f' },
	{ "threads", required_argument, NULL, 't' },
	{ "channels", required_argument, NULL, 'c' },
	{ NULL, 0, NULL, 0 }
    };

    unsigned int nthreads = 0;
    bool do_nfs = true;
    bool do_daemon = true;
#ifdef HAVE_UPNP
# ifdef HAVE_GSTREAMER
    bool do_audio = true;
# else
    bool do_audio = false;
# endif
    bool do_mserver = true;
#else
    bool do_audio = false;
    bool do_mserver = false;
#endif
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
    const char *software_server = NULL; // Null string means this host
    const char *channelsconf = "/etc/channels.conf";
    unsigned short port = 0; // Zero means pick an arbitrary one

    int option_index;
    int option;
    while ((option = getopt_long(argc, argv, "abrdhmnot:f:l:c:p:", options,
				 &option_index))
	   != -1)
    {
	switch (option)
	{
	case 'h':
	    Usage(stdout);
	    return 0;
	case 'n':
	    do_nfs = false;
	    break;
	case 'd':
	    do_daemon = false;
	    break;
	case 't':
	    nthreads = (unsigned int)strtoul(optarg, NULL, 10);
	    break;
	case 'p':
	    port = (unsigned short)strtoul(optarg, NULL, 10);
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
	do_receiver = false;
	do_dvb = false;
	do_mserver = false;
    }

    if (!do_audio && !do_receiver && !do_mserver && !do_cd)
    {
	printf("No protocols to serve; exiting\n");
	return 0;
    }

    if (do_daemon)
	daemonise();

    output::URLPlayer *player = NULL;
    if (do_audio)
    {
	// Start this off early, as it fork()s internally
	player = new output::gstreamer::URLPlayer;
    }

    if (!nthreads)
	nthreads = util::CountCPUs() * 2;

    const char *mediaroot = NULL;
    const char *flacroot  = NULL;
    if (do_db)
    {
	mediaroot = argv[optind];
	flacroot  = argv[optind+1];
	if (!flacroot)
	    flacroot = "";
    }

    util::WorkerThreadPool wtp(nthreads);

    Database db;
#ifdef HAVE_DB
    if (do_db)
	db.Init(mediaroot, flacroot, wtp.GetTaskQueue(), dbfile);
#endif

    TRACE << "serving\n";

    util::Poller poller;

    tv::dvb::Frontend dvbf;
    if (do_dvb)
    {
	unsigned rc = dvbf.Open(0,0);
	if (rc != 0)
	{
	    TRACE << "Can't open DVB frontend: " << rc << "\n";
	    do_dvb = false;
	}
    }

    tv::dvb::Channels dvbc;
    if (do_dvb)
    {
	unsigned rc = dvbc.Load(channelsconf);
	if (rc != 0)
	{
	    TRACE << "Can't load DVB channels from '" << channelsconf
		  << "': " << rc << "\n";
	    do_dvb = false;
	}
    }

    if (do_dvb)
    {
	if (dvbc.Count() == 0)
	{
	    TRACE << "No DVB channels found\n";
	    do_dvb = false;
	}
    }

    if (do_dvb)
    {
	dvbf.Tune(*dvbc.begin());

	int tries = 0;
	bool tuned;
	do {
	    dvbf.GetState(NULL, NULL, &tuned);
	    if (tuned)
		break;
	    sleep(1);
	    ++tries;
	} while (tries < 5);

	if (!tuned)
	{
	    TRACE << "Can't tune DVB frontend\n";
	    do_dvb = false;
	}
    }

    tv::RadioStreamFactory rsf(&dvbf, &dvbc);
    tv::dvb::EPG epg;

    if (do_dvb)
    {
	rsf.AddRadioStations(db.Get());
	db.Get()->RegisterStreamFactory(mediadb::RADIO, &rsf);
	epg.Init(&dvbf, &dvbc);
    }

    util::WebServer ws;

    receiverd::ContentFactory rcf(db.Get());
    util::FileContentFactory fcf(std::string(webroot)+"/upnp",
				 "/upnp");
    util::FileContentFactory fcf2(std::string(webroot)+"/layout",
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
    tv::RecordingThread rt;
    if (do_dvb)
	wepg.Init(&ws, epg.GetDatabase(), &dvbc, &dvbf, &rt, mediaroot);

    RootContentFactory rootcf(db.Get());

    ws.AddContentFactory("/", &rootcf);
    ws.Init(&poller, webroot, port);
    TRACE << "Webserver got port " << ws.GetPort() << "\n";

    receiver::ssdp::Server ssdp;
    if (do_receiver)
    {
	ssdp.Init(&poller);
	ssdp.RegisterService(receiver::ssdp::s_uuid_musicserver, ws.GetPort());
	if (do_nfs)
	    ssdp.RegisterService(receiver::ssdp::s_uuid_softwareserver, 111,
				 software_server);
	TRACE << "Receiver service started\n";
    }

    util::hal::Context *halp = NULL;

#ifdef HAVE_HAL
    util::dbus::Connection dbusc(&poller);
    unsigned int rc = dbusc.Connect(util::dbus::Connection::SYSTEM);
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

#ifdef HAVE_UPNP
    upnpd::MediaRenderer *mediarenderer = NULL;
    upnpd::MediaServer *mediaserver = NULL;
    upnp::Device *root_device = NULL;

    if (do_mserver)
    {
	mediaserver = new upnpd::MediaServer(db.Get(), ws.GetPort(), 
					     mediaroot);
	mediaserver->SetFriendlyName(std::string(mediaroot)
				     + " on " + hostname);
	root_device = mediaserver;
    }

# ifdef HAVE_GSTREAMER
    if (do_audio)
    {
	mediarenderer = new upnpd::MediaRenderer(player, "/dev/pcmC0D0p");
	mediarenderer->SetFriendlyName(std::string(hostname) + ":/dev/pcmC0D0p");
	if (root_device)
	    root_device->AddEmbeddedDevice(mediarenderer);
	else
	    root_device = mediarenderer;
    }
# endif

# ifdef HAVE_CD
    CDService cds(halp);
    if (do_cd)
	cds.Init(&ws, hostname, &root_device);
# endif

    upnp::Server svr;
    if (root_device)
	svr.Init(root_device, ws.GetPort());
#endif

    s_waker = new util::PollWaker(&poller, NULL);
    signal(SIGHUP, SignalHandler);
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    while (!s_exiting)
    {
	poller.Poll(util::Poller::INFINITE_MS);
	if (s_rescan && !s_exiting)
	{
	    s_rescan = false;
	    db.ForceRescan();
	}
    }

#ifdef HAVE_UPNP
    delete mediaserver;
    delete mediarenderer;
    delete player;
#endif

    // Care to avoid race against signal handler
    util::PollWaker *waker = s_waker;
    s_waker = NULL;
    delete waker;

    TRACE << "Orderly exit\n";

    return 0;
}

#endif // !WIN32
