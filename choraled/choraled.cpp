#include "config.h"

#ifndef WIN32

#include "libutil/trace.h"
#include "libutil/counted_pointer.h"
#include "cd.h"
#include "main.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/syslog.h>

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

/** Classes for the Chorale daemon (or Windows service)
 */
namespace choraled {

#if (HAVE_LIBCDIOP && !HAVE_PARANOIA)
#define LICENCE "  This program is free software under the GNU General Public Licence.\n\n"
#elif (HAVE_TAGLIB || HAVE_PARANOIA || HAVE_GSTREAMER || (HAVE_DVB && HAVE_MPG123))
#define LICENCE "  This program is free software under the GNU Lesser General Public Licence.\n\n"
#else
#define LICENCE "  This program is placed in the public domain by its authors.\n\n"
#endif

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
#if HAVE_GSTREAMER
" -a, --no-audio     Don't become a UPnP MediaRenderer server\n"
#endif
" -m, --no-mserver   Don't become a UPnP MediaServer server\n"
#if HAVE_CD
" -o, --no-optical   Don't serve optical (CD) drives for network ripping\n"
#endif
"     --assimilate-receiver  Gateway existing Receiver server to UPnP\n"
//"     --assimilate-empegs    Gateway networked Empegs to UPnP/Receivers\n"
//"     --assimilate-upnp      Gateway existing UPnP servers to Receivers\n"
#if HAVE_DVB
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
"  The option --assimilate-receiver implies -r.\n"
"  By default, the port used is " DEFAULT_WEB_PORT_S ", as used by 'traditional' Receiver\n"
"servers, unless -r is in effect, in which case the default port is 0, meaning\n"
"pick an arbitrary unused port. In either case, the default can be overridden\n"
"using -p.\n"
#if HAVE_LIBWRAP
"\n"
"  Use /etc/hosts.allow and /etc/hosts.deny to confer or deny access to\n"
"choraled, using the daemon-names 'choraled' or, for read-only access,\n"
"'choraled.ro'. See 'man hosts_access'.\n"
#endif
"\n"
"  Send SIGHUP to force a media re-scan (not needed on systems with 'inotify'\n"
"support).\n"
"\n"
	    LICENCE
"From " PACKAGE_STRING " (" PACKAGE_WEBSITE ") built on " __DATE__ ".\n"
	);
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

class UnixComplaints: public Complaints
{
public:
    ~UnixComplaints();

    void Complain(int loglevel, const char *format, ...);
};

UnixComplaints::~UnixComplaints()
{
}

void UnixComplaints::Complain(int loglevel, const char *format, ...)
{
    va_list args;
    va_start(args, format);
	    
    if (s_daemonised)
	vsyslog(loglevel, format, args);
    else
	vfprintf(stderr, format, args);
	
    va_end(args);
}

int ParseArgs(int argc, char *argv[], Settings *settings,
	      Complaints *complaints)
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
	{ "assimilate-receiver", no_argument, NULL, 11 },
	{ "web", required_argument, NULL, 'w' },
	{ "nfs", required_argument, NULL, 1 },
	{ "arf", required_argument, NULL, 2 },
	{ "dbfile", required_argument, NULL, 'f' },
	{ "threads", required_argument, NULL, 't' },
	{ "channels", required_argument, NULL, 'c' },
	{ "timer-db", required_argument, NULL, 3 },
	{ NULL, 0, NULL, 0 }
    };

    memset(settings, '\0', sizeof(settings));
    settings->flags = AUDIO | MEDIA_SERVER | RECEIVER | DVB | CD | LOCAL_DB;
    settings->database_file = DEFAULT_DB_FILE;
    settings->timer_database_file = DEFAULT_TIMER_DB_FILE;
    settings->web_root = DEFAULT_WEB_DIR;
    settings->receiver_software_server = NULL;
    settings->receiver_arf_file = DEFAULT_ARF_FILE;
    settings->dvb_channels_file = DEFAULT_CHANNELS_CONF;
    settings->nthreads = 32;

    bool explicit_port = false;
    bool do_daemon = true;

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
	    exit(0);
	    return 0;
	case 'd':
	    do_daemon = false;
	    break;
	case 't':
	    settings->nthreads = (unsigned int)strtoul(optarg, NULL, 10);
	    break;
	case 'p':
	    settings->web_port = (unsigned short)strtoul(optarg, NULL, 10);
	    explicit_port = true;
	    break;
	case 'f':
	    settings->database_file = optarg;
	    break;
	case 'w':
	    settings->web_root = optarg;
	    break;
	case 'c':
	    settings->dvb_channels_file = optarg;
	    break;
	case 'a':
	    settings->flags &= ~AUDIO;
	    break;
	case 'm':
	    settings->flags &= ~MEDIA_SERVER;
	    break;
	case 'o':
	    settings->flags &= ~CD;
	    break;
	case 'r':
	    settings->flags &= ~RECEIVER;
	    break;
	case 'b':
	    settings->flags &= ~DVB;
	    break;
	case 1:
	    settings->receiver_software_server = optarg;
	    break;
	case 2:
	    settings->receiver_arf_file = optarg;
	    break;
	case 3:
	    settings->timer_database_file = optarg;
	    break;
	case 11:
	    settings->flags |= ASSIMILATE_RECEIVER;
	    settings->flags &= ~RECEIVER;
	    break;
	default:
	    Usage(stderr);
	    exit(1);
	    return 0;
	}
    }

    switch ((int)(argc-optind)) // How many more arguments?
    {
    case 2:
	settings->media_root = argv[optind];
	settings->flac_root  = argv[optind+1];
	break;
    case 1:
	settings->media_root = argv[optind];
	settings->flac_root  = "";
	break;
    case 0:
	// No music directory given -- disable all file ops
	settings->flags &= ~LOCAL_DB;
	break;
    default:
	Usage(stderr);
	exit(1);
	return 1;
    }

    if ((settings->flags & (LOCAL_DB | DVB | ASSIMILATE_RECEIVER)) == 0)
    {
	// Nothing to serve
	settings->flags &= ~MEDIA_SERVER;
	settings->flags &= ~RECEIVER;
    }

    if ((settings->flags & (AUDIO | RECEIVER | MEDIA_SERVER | CD)) == 0)
    {
	complaints->Complain(LOG_WARNING, "No protocols to serve; exiting\n");
	return 0;
    }

    if (!explicit_port)
    {
	if (settings->flags & RECEIVER)
	    settings->web_port = DEFAULT_WEB_PORT;
	else
	    settings->web_port = 0;
    }

    if (do_daemon)
	daemonise();

    return 0;
}

} // namespace choraled

int main(int argc, char *argv[])
{
    choraled::Settings settings;
    choraled::UnixComplaints complaints;

    choraled::ParseArgs(argc, argv, &settings, &complaints);

    return choraled::Main(&settings, &complaints);
}

#endif // !WIN32
