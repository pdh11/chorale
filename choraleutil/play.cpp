#include "config.h"
#include "liboutput/queue.h"
#include "liboutput/gstreamer.h"
#include "libimport/tags.h"
#include "libdbsteam/db.h"
#include "libmediadb/schema.h"
#include "libdblocal/db.h"
#include "libdb/recordset.h"
#include "libutil/trace.h"
#include "libutil/http_client.h"
#include <getopt.h>
#include <stdio.h>

void Usage(FILE *f)
{
    fprintf(f,
	    "Usage: play <file>...\n\n"
	    "    Plays back audio files.\n"
	    "\n"
	    "From chorale " PACKAGE_VERSION " built on " __DATE__ ".\n"
	);
}

int main(int argc, char *argv[])
{
    static const struct option options[] =
    {
	{ "help",  no_argument, NULL, 'h' },
	{ NULL, 0, NULL, 0 }
    };

    int option_index;
    int option;
    while ((option = getopt_long(argc, argv, "h", options, &option_index))
	   != -1)
    {
	switch (option)
	{
	case 'h':
	    Usage(stdout);
	    return 0;
	default:
	    Usage(stderr);
	    return 1;
	}
    }

    if (argc == optind)
    {
	Usage(stderr);
	return 1;
    }

#if HAVE_TAGLIB
    db::steam::Database sdb(mediadb::FIELD_COUNT);

    for (int i=optind; i<argc; ++i)
    {
	db::RecordsetPtr rs = sdb.CreateRecordset();
	rs->AddRecord();
	
	import::Tags tags;
	unsigned int rc = tags.Open(argv[i]);
	if (rc == 0)
	    rc = tags.Read(rs.get());
	if (rc == 0)
	{
	    rs->SetInteger(mediadb::TYPE, mediadb::TUNE);
	    rs->SetInteger(mediadb::ID, 0x100 + i);
	    rs->SetString(mediadb::PATH, argv[i]);
	    rs->Commit();
	}
	else
	{
	    TRACE << "Don't like " << argv[i] << "\n";
	}
    }

    util::http::Client http_client;
    db::local::Database ldb(&sdb, &http_client);
#endif

#if HAVE_GSTREAMER
    output::gstreamer::URLPlayer gp;

    unsigned int rc = gp.Init();
    if (rc)
    {
	TRACE << "Can't init gstreamer: " << rc << "\n";
	return 1;
    }

#if HAVE_TAGLIB
    output::Queue queue(&gp);
    for (int i = optind; i<argc; ++i)
    {
	queue.Add(&ldb, 0x100 + i);
    }

    queue.SetPlayState(output::PLAY);
#else
    gp.SetURL(std::string("file://") + argv[optind], "");
    if (argv[optind+1])
	gp.SetNextURL(std::string("file://") + argv[optind+1], "");
    gp.SetPlayState(output::PLAY);
#endif

    while (1)
    {}

#else
    fprintf(stderr, "No playback available -- GStreamer not found\n");
#endif

    return 0;
}
