#include "config.h"
#include "liboutput/queue.h"
#include "liboutput/urlplayer.h"
#include "libimport/tags.h"
#include "libdbsteam/db.h"
#include "libmediadb/schema.h"
#include "libmediadb/localdb.h"
#include "libutil/trace.h"
#include <getopt.h>

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

    db::steam::Database sdb(mediadb::FIELD_COUNT);

    for (int i=optind; i<argc; ++i)
    {
	db::RecordsetPtr rs = sdb.CreateRecordset();
	rs->AddRecord();
	
	import::TagsPtr tags = import::Tags::Create(argv[i]);
	
	unsigned int rc = tags->Read(rs);
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

    mediadb::LocalDatabase ldb(&sdb);

    output::GSTPlayer gp;

    sleep(1);

    gp.SetURL(std::string("file://") + argv[optind], "");
    gp.SetPlayState(output::PLAY);

    while (1);

    output::Queue queue(&gp);
    for (int i = optind; i<argc; ++i)
    {
	queue.Add(&ldb, 0x100 + i);
    }
    queue.SetPlayState(output::PLAY);

    return 0;
}
