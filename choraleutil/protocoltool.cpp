#include "config.h"
#include "libdbempeg/db.h"
#include "libmediadb/xml.h"
#include "libmediadb/sync.h"
#include "libmediadb/schema.h"
#include "libdblocal/db.h"
#include "libdbsteam/db.h"
#include "libdblocal/file_scanner.h"
#include "libdb/query.h"
#include "libdb/recordset.h"
#include "libempeg/discovery.h"
#include "libempeg/protocol_client.h"
#include "libutil/scheduler.h"
#include "libutil/socket.h"
#include "libutil/file_stream.h"
#include "libutil/http_server.h"
#include "libutil/http_client.h"
#include "libutil/worker_thread_pool.h"
#include <getopt.h>
#include <stdlib.h>
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

/** Command-line program for Empeg protocol operations (eg for car-players).
 */
namespace protocoltool {

static bool dry_run = false;
static bool do_scan = false;
static bool do_full_scan = false;
static bool do_restart = false;
static bool do_reboot = false;
static bool do_update = false;
static const char *update_root = NULL;
static const char *update_flac_root = NULL;
static const char *update_xml = NULL;
static util::IPAddress s_device = util::IPAddress::ANY;

static void Usage(FILE *f)
{
    fprintf(f,
	 "Usage: protocoltool [options] [<root-directory> [<flac-root-directory>]]\n"
"Control an \"Empeg Protocol\" network device: a car-player or Rio Central.\n"
"\n"
"The options are:\n"
" -d, --device=IP    Select device (rather than picking one at random)\n"
" -s, --scan         Scan network for compatible devices\n"
" -f, --full-scan      Display full information about each device\n"
" -r, --restart      Orderly exit and re-start of player firmware\n"
"     --reboot       Completely reboot kernel\n"
" -u, --update       Update (synchronise) device to match local database\n"
" -n, --dry-run        Don't actually update the device, just print details\n"
"     --xml=FILE       Use Chorale database dump FILE instead of scanning files\n"
" -h, --help         These hastily-scratched notes\n"
	    "\n"

"  Supplying -d is required for -u. Supplying <root-directory> is also\n"
"required for -u, unless --xml is used. The <flac-root-directory> is assumed\n"
"to have the same layout as <root-directory> but with FLACs instead of MP3s;\n"
"if not given, no FLACs are transferred, which is what you want with v2\n"
"car-players as they can't play them anyway."

"\n"
"From " PACKAGE_STRING " (" PACKAGE_WEBSITE ") built on " __DATE__ ".\n"
	);
}

static void Version()
{
    printf("protocoltool, part of " PACKAGE_STRING ".\n");
}


        /* FindDevice */


/** Callback that just picks the first device to respond.
 */
struct FindDeviceCallback: public empeg::Discovery::Callback
{
    void OnDiscoveredEmpeg(const util::IPAddress& ip, const std::string& name)
    {
	s_device = ip;
	printf("Using %s on %s\n", name.c_str(), ip.ToString().c_str());
    }
};

/** Assuming there's only one device on the network, find it and store its
 * IP address in s_device.
 */
void FindDevice()
{
    util::BackgroundScheduler poller;
    empeg::Discovery disc;
    FindDeviceCallback sc;
    disc.Init(&poller, &sc);

    time_t t = time(NULL) + 5;

    while (time(NULL) < t && s_device != util::IPAddress::ANY)
	poller.Poll(1000);

    if (s_device == util::IPAddress::ANY)
    {
	fprintf(stderr, "No devices found\n");
	exit(1);
    }
}


        /* Scan */


/** Callback that lists all responding devices.
 */
struct ScanCallback: public empeg::Discovery::Callback
{
    bool any;

    ScanCallback() : any(false) {}

    void OnDiscoveredEmpeg(const util::IPAddress& ip, const std::string& name);
};

void ScanCallback::OnDiscoveredEmpeg(const util::IPAddress& ip, 
				     const std::string& name)
{
    printf("%-15s %s\n", ip.ToString().c_str(), name.c_str());
    if (do_full_scan)
    {
	empeg::ProtocolClient pc;
	unsigned int rc = pc.Init(ip);
	if (rc != 0)
	{
	    printf("%-15s ** Can't contact: error %u\n", "", rc);
	    return;
	}
	std::string version, id, type, configini;
	rc = pc.ReadFidToString(empeg::FID_VERSION, &version);
	if (!rc)
	    rc = pc.ReadFidToString(empeg::FID_ID, &id);
//	if (!rc)
//	    rc = pc.ReadFidToString(empeg::FID_CONFIGINI, &configini);
	if (!rc)
	    rc = pc.ReadFidToString(empeg::FID_TYPE, &type);
	if (rc)
	{
	    printf("%-15s ** Can't collect info: error %u\n", "", rc);
	    return;
	}
	version.erase(version.find_last_not_of(" \t\n")+1);

	const char *idstr = strstr(id.c_str(), "hwrev :");
	unsigned int hwrev = 0;
	if (idstr)
	    hwrev = atoi(idstr + 7);
	idstr = strstr(id.c_str(), "serial:");
	unsigned int serial = 0;
	if (idstr)
	    serial = atoi(idstr + 7);

	if (any)
	    printf("\n");
	any = true;

	if (!strcmp(type.c_str(), "jupiter"))
	{
	    // Holy hell, it's a Rio Central/HSX-109. Not many of them around.

	    printf("%-15s Jupiter #%06u v%s hwrev %u\n",
		   "",
		   serial % 1000000,
		   version.c_str(),
		   hwrev);
	}
	else
	{
	    // Mark 2 serial numbers are decimal mmyynnnnn
	    //   mm = month of manufacture 01-12
	    //   yy = year of manufacture 00-01
	    //   nnnnn = actual serial number starting at 1
	    //
	    // This means that the very first Mark 2 was #060000001, and
	    // also that serial numbers order isn't chronological order:
	    // 120001100 was manufactured before 010101200 despite being
	    // numerically larger.
	    
	    printf("%-15s Mark %-2s #%05u (%02u/%u) v%s\n",
		   "",
		   (hwrev < 9) ? "2" : "2a", // No Mark 1s on ethernet!
		   serial   % 100000,        // We only planned 100,000 ever!
		   (serial  / 10000000),             // Month of manufacture
		   ((serial / 100000) % 100) + 2000, // Year of manufacture
		   version.c_str());
	}

//	printf("version: '%s'\n", version.c_str());
//	printf("type: '%s'\n", type.c_str());
//	printf("id: >>>%s\n<<<\n", id.c_str());
//	printf("config: >>>%s\n<<<\n", configini.c_str());
    }
    else
	any = true;
}

static int Scan()
{
    util::BackgroundScheduler poller;
    empeg::Discovery disc;
    ScanCallback sc;
    disc.Init(&poller, &sc);

    time_t t = time(NULL);

    while ((time(NULL) - t) < 5)
    {
	poller.Poll(1000);
    }

    if (!sc.any)
    {
	printf("No devices found\n");
	return 1;
    }

    return 0;
}


        /* Update */


static void PrintSeconds(unsigned long long sec)
{
    if (sec == 0)
	sec = 1;

    if (sec > 3600*24)
    {
	printf("%llud", sec/(3600*24));
	sec %= (3600*24);
    }
    if (sec > 3600)
    {
	printf("%lluh", sec/3600);
	sec %= 3600;
    }
    if (sec > 60)
    {
	printf("%llum", sec/60);
	sec %= 60;
    }
    if (sec)
	printf("%llus", sec);
}

class UpdateObserver: public mediadb::SyncObserver
{
    db::Database *m_src;
    db::Database *m_dest;

    void Print(db::Database *db, const char *verb, unsigned int id,
	       size_t num, size_t denom)
    {
	db::QueryPtr qp = db->CreateQuery();
	qp->Where(qp->Restrict(mediadb::ID, db::EQ, id));
	db::RecordsetPtr rs = qp->Execute();
	if (rs && !rs->IsEOF())
	{
	    printf("%s %u/%u %s\n", verb,
		   (unsigned int)num+1,
		   (unsigned int)denom, 
		   rs->GetString(mediadb::TITLE).c_str());
	}
    }

public:
    UpdateObserver(db::Database *src, db::Database *dest)
	: m_src(src), m_dest(dest) {}
	
    void OnAddFile(unsigned int srcid, size_t num, size_t denom)
    {
	Print(m_src, "Adding", srcid, num, denom);
    }

    void OnAddPlaylist(unsigned int srcid, size_t num, size_t denom)
    {
	Print(m_src, "Adding playlist", srcid, num, denom);
    }

    void OnAmendPlaylist(unsigned int srcid, size_t num, size_t denom)
    {
	Print(m_src, "Updating playlist", srcid, num, denom);
    }

    void OnAmendMetadata(unsigned int srcid, size_t num, size_t denom)
    {
	Print(m_src, "Updating", srcid, num, denom);
    }
};

static int Update()
{
    util::WorkerThreadPool wtp(util::WorkerThreadPool::NORMAL, 32);

    // Prepare source database
    db::steam::Database sdbsrc(mediadb::FIELD_COUNT);
    sdbsrc.SetFieldInfo(mediadb::ID, 
		     db::steam::FIELD_INT|db::steam::FIELD_INDEXED);
    sdbsrc.SetFieldInfo(mediadb::PATH,
		     db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
    sdbsrc.SetFieldInfo(mediadb::TITLE,
		     db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
    sdbsrc.SetFieldInfo(mediadb::TRACKNUMBER,
		     db::steam::FIELD_INT|db::steam::FIELD_INDEXED);

    util::http::Client http_client;
    db::local::Database dbsrc(&sdbsrc, &http_client);

    unsigned int rc;

    if (update_xml)
    {
	std::auto_ptr<util::Stream> fsp;
	rc = util::OpenFileStream(update_xml, util::READ, &fsp);
	if (rc)
	{
	    fprintf(stderr, "Can't open %s: %u\n", update_xml, rc);
	    return 1;
	}
	rc = mediadb::ReadXML(&dbsrc, fsp.get());
	if (rc)
	{
	    fprintf(stderr, "Can't parse %s: %u\n", update_xml, rc);
	    return 1;
	}
    }
    else
    {
	db::local::FileScanner ifs(update_root, update_flac_root, &sdbsrc,
				   &dbsrc, &wtp);

	rc = ifs.Scan();
	if (rc)
	{
	    fprintf(stderr, "Can't scan %s: %u\n", update_root, rc);
	    return 1;
	}
    }

    // Prepare destination database
    util::BackgroundScheduler poller;
    util::http::Server server(&poller, &wtp);
    db::empeg::Database dbdest(&server);
    rc = dbdest.Init(s_device);
    if (rc != 0)
    {
	fprintf(stderr, "Can't contact %s: error %u\n", 
		s_device.ToString().c_str(), rc);
	return 1;
    }

    struct timeval tv;
    ::gettimeofday(&tv, NULL);
    uint64_t usec = (((uint64_t)tv.tv_sec) * 1000000) + tv.tv_usec;

    // Perform sync
    mediadb::Synchroniser sync(&dbsrc, &dbdest, dry_run);
    UpdateObserver uobs(&dbsrc, &dbdest);
    sync.AddObserver(&uobs);

    rc = sync.Synchronise();
    if (rc != 0)
    {
	fprintf(stderr, "Synchronise failed: error %u\n", rc);
	return 1;
    }

    ::gettimeofday(&tv, NULL);
    usec = (((uint64_t)tv.tv_sec) * 1000000) + tv.tv_usec - usec;

    mediadb::Synchroniser::Statistics s = sync.GetStatistics();
    if (dry_run)
    {
	printf("Would transfer %u files, %u playlists, %llu bytes;\n"
	       "  delete %u files, %u playlists; amend %u files and %u playlists.\n",
	       s.files_added, s.playlists_added, s.bytes_transferred,
	       s.files_deleted, s.playlists_deleted, s.files_modified,
	       s.playlists_modified);
	unsigned long long estimated_sec = s.bytes_transferred / 750000;
	if (estimated_sec)
	{
	    printf("Estimated sync time ");
	    PrintSeconds(estimated_sec);
	    printf("\n");
	}
    }
    else
    {
	printf("Transferred %u files, %u playlists, %llu bytes;\n"
	       "  deleted %u files, %u playlists; amended %u files and %u playlists.\n",
	       s.files_added, s.playlists_added, s.bytes_transferred,
	       s.files_deleted, s.playlists_deleted, s.files_modified,
	       s.playlists_modified);
	printf("Sync time ");
	PrintSeconds(usec/1000000);
	printf("\n");
    }

    return 0;
}


        /* Restart/reboot */


static int QuitPlayer(unsigned int (empeg::ProtocolClient::*f)(void))
{
    empeg::ProtocolClient pc;
    unsigned int rc = pc.Init(s_device);
    if (!rc)
	rc = (pc.*f)();
    if (rc)
    {
	fprintf(stderr, "Can't contact: error %u\n", rc);
	return 1;
    }
    printf("OK\n");
    return 0;
}


        /* Main program */


int Main(int argc, char *argv[])
{
    static const struct option options[] =
    {
	{ "help", no_argument, NULL, 'h' },
	{ "version", no_argument, NULL, 'v' },
	{ "device", required_argument, NULL, 'd' },
	{ "update", no_argument, NULL, 'u' },
	{ "dry-run", no_argument, NULL, 'n' },
	{ "scan", no_argument, NULL, 's' },
	{ "full-scan", no_argument, NULL, 'f' },
	{ "restart", no_argument, NULL, 'r' },
	{ "reboot", no_argument, NULL, 1 },
	{ "xml", required_argument, NULL, 2 }
    };

    int option_index;
    int option;
    while ((option = getopt_long(argc, argv, "hvsfnd:ur", options,
				 &option_index))
	   != -1)
    {
	switch (option)
	{
	case 'h':
	    Usage(stdout);
	    return 0;
	case 'v':
	    Version();
	    return 0;
	case 'd':
	    s_device = util::IPAddress::Resolve(optarg);
	    break;
	case 'n':
	    dry_run = true;
	    break;
	case 'u':
	    do_update = true;
	    break;
	case 's':
	    do_scan = true;
	    break;
	case 'f':
	    do_scan = true;
	    do_full_scan = true;
	    break;
	case 'r':
	    do_restart = true;
	    break;
	case 1:
	    do_reboot = true;
	    break;
	case 2:
	    update_xml = optarg;
	    break;
	default:
	    Usage(stderr);
	    return 1;
	}
    }

    if (do_update && !update_xml)
    {
	if (argc > optind+2 || argc <= optind)
	{
	    Usage(stderr);
	    return 1;
	}
	update_root      = argv[optind];
	update_flac_root = argv[optind+1];
    }
    else
    {
	if (argc != optind)
	{
	    Usage(stderr);
	    return 1;
	}
    }

    if (!do_scan && !do_update && !do_restart && !do_reboot)
    {
	Usage(stdout);
	return 0;
    }

    if (do_scan)
	return Scan();

    if (do_restart)
    {
	if (s_device == util::IPAddress::ANY)
	    FindDevice();
	return QuitPlayer(&empeg::ProtocolClient::RestartPlayer);
    }

    if (do_reboot)
    {
	if (s_device == util::IPAddress::ANY)
	    FindDevice();
//	return QuitPlayer(&empeg::ProtocolClient::Reboot);
    }

    if (do_update)
    {
	if (s_device == util::IPAddress::ANY)
	{
	    fprintf(stderr, "Specifying --device is necessary for --update. (Hint: --scan.)\n");
	    return 1;
	}
	if (!update_xml && !update_root)
	{
	    fprintf(stderr, "Specifying either --xml or a root directory is necessary for --update.\n");
	    return 1;
	}
	return Update();
    }
    
    return 0;
}

} // namespace protocoltool

int main(int argc, char *argv[])
{
    return protocoltool::Main(argc, argv);
}
