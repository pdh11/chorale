#include "config.h"
#include <getopt.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sstream>
#include <iomanip>
#include <vector>
#include <stdio.h>
#include "libutil/worker_thread_pool.h"
#include "libutil/http_client.h"
#include "libutil/trace.h"
#include "libdbsteam/db.h"
#include "libmediadb/schema.h"
#include "libdblocal/db.h"
#include "libdblocal/file_scanner.h"

void Usage(FILE *f)
{
    fprintf(f,
	 "Usage: readtags [-q] [-t n] [-s] <dir> [<flac-dir>]\n\n"
"    Extracts available metadata from files or directories.\n"
"    With -q, does the extraction but prints nothing (for timing tests)\n"
"    With -t, uses n threads for the extraction.\n"
"    Built on " __DATE__ ".\n"
	);
}

bool quiet = false;

int main(int argc, char *argv[])
{
    unsigned int nthreads = 2;

    static const struct option options[] =
    {
	{ "help",  no_argument, NULL, 'h' },
	{ "quiet", no_argument, NULL, 'q' },
	{ "threads", required_argument, NULL, 't' },
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
	    nthreads = (unsigned int)strtoul(optarg, NULL, 10);
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

    util::WorkerThreadPool wtp(util::WorkerThreadPool::NORMAL, nthreads);
    util::http::Client http_client;
    
    db::steam::Database sdb(mediadb::FIELD_COUNT);
    sdb.SetFieldInfo(mediadb::ID, 
		     db::steam::FIELD_INT|db::steam::FIELD_INDEXED);
    sdb.SetFieldInfo(mediadb::PATH,
		     db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);

    const char *flacdir = argv[optind+1];
    if (!flacdir)
	flacdir = "";

    db::local::Database ldb(&sdb, &http_client);

#if HAVE_TAGLIB
    db::local::FileScanner ifs(argv[optind], flacdir, &ldb, &wtp);

    unsigned int rc = ifs.Scan();
    assert(rc == 0);
#endif

    return 0;
}
