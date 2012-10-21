#include "config.h"
#include <getopt.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sstream>
#include <iomanip>
#include <vector>
#include "libutil/worker_thread_pool.h"
#include <boost/thread/mutex.hpp>
#include "libutil/trace.h"
#include "libdbsqlite/db.h"
#include "libdbsteam/db.h"
#include "libimport/file_scanner.h"
#include "libmediadb/schema.h"
#include "libmediadb/localdb.h"

void Usage(FILE *f)
{
    fprintf(f,
	 "Usage: readtags [-q] [-t n] [-s] <dir> [<flac-dir>]\n\n"
"    Extracts available metadata from files or directories.\n"
"    With -q, does the extraction but prints nothing (for timing tests)\n"
"    With -t, uses n threads for the extraction.\n"
#ifdef HAVE_SQLITE
"    With -s, uses Sqlite (otherwise steamdb).\n"
#endif
"    Built on " __DATE__ ".\n"
	);
}

bool quiet = false;
bool sqlite = false;

int main(int argc, char *argv[])
{
    unsigned int nthreads = 2;

    static const struct option options[] =
    {
	{ "help",  no_argument, NULL, 'h' },
	{ "quiet", no_argument, NULL, 'q' },
	{ "threads", required_argument, NULL, 't' },
#ifdef HAVE_SQLITE
	{ "sqlite", no_argument, NULL, 's' },
#endif
	{ NULL, 0, NULL, 0 }
    };

    int option_index;
    int option;
    while ((option = getopt_long(argc, argv, "hqt:s", options, &option_index))
	   != -1)
    {
	switch (option)
	{
	case 'h':
	    Usage(stdout);
	    return 0;
	case 'q':
	    quiet = true;
	    break;
	case 't':
	    nthreads = strtoul(optarg, NULL, 10);
	    break;
	case 's':
	    sqlite = true;
	    break;
	default:
	    Usage(stderr);
	    return 1;
	}
    }

    if (optind == argc || nthreads < 1)
    {
	Usage(stderr);
	return 1;
    }

    util::WorkerThreadPool wtp(nthreads);

    db::Database *thedb;
#ifdef HAVE_SQLITE
    db::sqlite::Database *sqlitedb = NULL;
    if (sqlite)
    {
	unlink("readtags.db");
	sqlitedb = new db::sqlite::Database;
	sqlitedb->Open("readtags.db");
	sqlitedb->SetAutoFlush(false);
	thedb = sqlitedb;
    }
    else
#endif
    {
	thedb = new db::steam::Database(mediadb::FIELD_COUNT);
    }

    const char *flacdir = argv[optind+1];
    if (!flacdir)
	flacdir = "";

    mediadb::LocalDatabase ldb(thedb);

#ifdef HAVE_TAGLIB
    import::FileScanner ifs(argv[optind], flacdir, &ldb, wtp.GetTaskQueue());

    unsigned int rc = ifs.Scan();
    assert(rc == 0);
#endif

#ifdef HAVE_SQLITE
    if (sqlite)
	sqlitedb->SetAutoFlush(true);
#endif

    delete thedb;

    return 0;
}
