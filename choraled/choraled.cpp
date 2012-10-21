#include "config.h"
#include "libdbsteam/db.h"
#include "libimport/file_scanner_thread.h"
#include "libmediadb/schema.h"
#include "libmediadb/localdb.h"
#include "libutil/worker_thread_pool.h"
#include "libutil/cpus.h"
#include "libutil/web_server.h"
#include "libutil/trace.h"
#include "libutil/poll.h"
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
#define DEFAULT_WEB_DIR DATADIR "/chorale"

static void Usage(FILE *f)
{
    fprintf(f,
	 "Usage: choraled [options] <root-directory> [<flac-root-directory>]\n"
"Serve media from given directory, to network clients\n"
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
//" -i, --interface=IF Network interface to listen on (default=first found)\n"
#ifdef HAVE_UPNP
#ifdef HAVE_GSTREAMER
" -a, --no-audio     Don't become a UPnP MediaRenderer server\n"
#endif
" -m, --no-mserver   Don't become a UPnP MediaServer server\n"
#endif
#ifdef HAVE_DVB
" -b, --no-broadcast Don't serve DVB channels\n"
" -c, --channels=FILE  Use FILE as DVB channel list (default=/etc/channels.conf)\n"
#endif
	    "\n"
	    "Send SIGHUP to force a media re-scan (not needed on systems with 'inotify'\n"
	    "support).\n"
	    "\n"
"From chorale " PACKAGE_VERSION " built on " __DATE__ ".\n"
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
	{ "threads", required_argument, NULL, 't' },
	{ "dbfile", required_argument, NULL, 'f' },
	{ "web", required_argument, NULL, 'w' },
	{ "no-daemon", no_argument, NULL, 'd' },
	{ "nfs", required_argument, NULL, 1 },
	{ "no-receiver", no_argument, NULL, 'r' },
	{ "no-audio", no_argument, NULL, 'a' },
	{ "no-mserver", no_argument, NULL, 'm' },
	{ "no-broadcast", no_argument, NULL, 'b' },
	{ "channels", required_argument, NULL, 'c' },
	{ NULL, 0, NULL, 0 }
    };

    int nthreads = 0;
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
    const char *dbfile = DEFAULT_DB_FILE;
    const char *webroot = DEFAULT_WEB_DIR;
    const char *software_server = NULL; // Null string means this host
    const char *channelsconf = "/etc/channels.conf";

    int option_index;
    int option;
    while ((option = getopt_long(argc, argv, "abrdhmnt:f:l:c:", options,
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
	    nthreads = strtoul(optarg, NULL, 10);
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

    if (argc <= optind || argc > optind+2 || nthreads < 0)
    {
	Usage(stderr);
	return 1;
    }

    if (!do_audio && !do_receiver && !do_mserver)
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

    const char *mediaroot = argv[optind];
    const char *flacroot  = argv[optind+1];
    if (!flacroot)
	flacroot = "";

    db::steam::Database sdb(mediadb::FIELD_COUNT);
    sdb.SetFieldInfo(mediadb::ID, 
		     db::steam::FIELD_INT|db::steam::FIELD_INDEXED);
    sdb.SetFieldInfo(mediadb::PATH,
		     db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
    sdb.SetFieldInfo(mediadb::ARTIST,
		     db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
    sdb.SetFieldInfo(mediadb::ALBUM,
		     db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
    sdb.SetFieldInfo(mediadb::GENRE,
		     db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
    sdb.SetFieldInfo(mediadb::TITLE,
		     db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);

    util::WorkerThreadPool wtp(nthreads);

    import::FileScannerThread ifs;

    mediadb::LocalDatabase ldb(&sdb);

    ifs.Init(mediaroot, flacroot, &ldb, wtp.GetTaskQueue(), dbfile);

    TRACE << "serving\n";

    util::Poller poller;

    receiver::ssdp::Server ssdp;

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
	rsf.AddRadioStations(&ldb);
	ldb.RegisterStreamFactory(mediadb::RADIO, &rsf);
	epg.Init(&dvbf, &dvbc);
    }

    util::WebServer ws;

    receiverd::ContentFactory rcf(&ldb);
    util::FileContentFactory fcf(std::string(webroot)+"/layout",
				 "/layout");

    if (do_receiver || do_mserver)
    {
	ws.AddContentFactory("/tags", &rcf);
	ws.AddContentFactory("/query", &rcf);
	ws.AddContentFactory("/content", &rcf);
	ws.AddContentFactory("/results", &rcf);
	ws.AddContentFactory("/list", &rcf);
	ws.AddContentFactory("/layout", &fcf);
    }

    tv::WebEPG wepg;
    tv::RecordingThread rt;
    if (do_dvb)
	wepg.Init(&ws, epg.GetDatabase(), &dvbc, &dvbf, &rt, mediaroot);

    RootContentFactory rootcf;

    ws.AddContentFactory("/", &rootcf);
    ws.Init(&poller, webroot, 0);
    TRACE << "Webserver got port " << ws.GetPort() << "\n";

    if (do_receiver)
    {
	ssdp.Init(&poller);
	ssdp.RegisterService(receiver::ssdp::s_uuid_musicserver, ws.GetPort());
	if (do_nfs)
	    ssdp.RegisterService(receiver::ssdp::s_uuid_softwareserver, 111,
				 software_server);
    }

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
    upnp::Server *svr = NULL;

    if (do_mserver)
    {
	mediaserver = new upnpd::MediaServer(&ldb, ws.GetPort(), mediaroot);
	mediaserver->SetFriendlyName(std::string(hostname) + ":" + mediaroot);
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

    if (root_device)
    {
	svr = new upnp::Server;
	svr->Init(root_device, ws.GetPort());
    }
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
	    ifs.ForceRescan();
	}
    }

#ifdef HAVE_UPNP
    delete svr;
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
