#include "config.h"
#include "libdbsteam/db.h"
#include "libimport/file_scanner.h"
#include "libmediadb/schema.h"
#include "libmediadb/xml.h"
#include "libmediadb/localdb.h"
#include "libutil/worker_thread_pool.h"
#include "libutil/cpus.h"
#include "libutil/trace.h"
#include "libutil/file.h"
#include <boost/format.hpp>
#include <getopt.h>

#define DEFAULT_DB_FILE "/var/chorale/db.xml"

static void Usage(FILE *f)
{
    fprintf(f,
	 "Usage: squirt [options] <output-dir> <root-dir> [<flac-root-dir>]\n"
"Fill output-dir with an Empeg car-player file structure for copying the\n"
"given music collection.\n"
"\n"
"The options are:\n"
" -f, --dbfile=FILE  Database file (default=" DEFAULT_DB_FILE "\n"
" -h, --help         These hastily-scratched notes\n"
" -t, --threads=N    Use N threads to scan files (default NCPUS*2)\n"
	    "\n"
"From chorale " PACKAGE_VERSION " built on " __DATE__ ".\n"
	);
}

typedef std::map<unsigned int, unsigned int> fidmap_t;

static fidmap_t fidmap;
static unsigned int next_fid = 0x120;

void PreAllocateFIDs(db::Database *thedb, int type)
{
    db::QueryPtr qp = thedb->CreateQuery();
    qp->Where(qp->Restrict(mediadb::TYPE, db::EQ, type));
    db::RecordsetPtr rs = qp->Execute();

    while (rs && !rs->IsEOF())
    {
	unsigned int fid = rs->GetInteger(mediadb::ID);
	if (fidmap.find(fid) == fidmap.end())
	{
	    fidmap[fid] = next_fid;
	    next_fid += 0x10;
	}
	rs->MoveNext();
    }
}

static const char *const typemap[] = {
    "",
    "tune",
    "tune",
    "tune",
    "playlist",
    "playlist",
    "illegal",
    "illegal",
    "illegal",
    "illegal"
};

enum { NTYPES = sizeof(typemap)/sizeof(typemap[0]) };

BOOST_STATIC_ASSERT((int)NTYPES == (int)mediadb::TYPE_COUNT);

static const char *const codecmap[] = {
    "",
    "mp2",
    "mp3",
    "flac",
    "vorbis",
    "wav",
    "pcm",
    "jpeg"
};

enum { NCODECS = sizeof(codecmap)/sizeof(codecmap[0]) };

BOOST_STATIC_ASSERT((int)NCODECS == (int)mediadb::CODEC_COUNT);

static void MaybeWriteString(FILE *f, db::RecordsetPtr rs, const char *tag,
			     unsigned int field)
{
    std::string val = rs->GetString(field);
    if (val.empty())
	return;
    fprintf(f, "%s=%s\n", tag, val.c_str());
}

static void MaybeWriteInt(FILE *f, db::RecordsetPtr rs, const char *tag,
			  unsigned int field)
{
    unsigned int val = rs->GetInteger(field);
    if (!val)
	return;
    fprintf(f, "%s=%u\n", tag, val);
}

void WriteFIDStructure(const char *outputdir, db::Database *thedb)
{
    fidmap[0x100] = 0x100;

    // Allocate tunes first, to cause minimum pressure on dynamic data area
    PreAllocateFIDs(thedb, mediadb::TUNE);
    PreAllocateFIDs(thedb, mediadb::SPOKEN);
    printf("Highest tune fid: 0x%x\n", next_fid - 0x10);
    PreAllocateFIDs(thedb, mediadb::DIR);
    PreAllocateFIDs(thedb, mediadb::PLAYLIST);
    printf("Highest fid: 0x%x\n", next_fid - 0x10);

    uint64_t drivesize[2];
    drivesize[0] = 0;
    drivesize[1] = 0;

    for (fidmap_t::const_iterator i = fidmap.begin(); i != fidmap.end(); ++i)
    {
	db::QueryPtr qp = thedb->CreateQuery();
	qp->Where(qp->Restrict(mediadb::ID, db::EQ, i->first));
	db::RecordsetPtr rs = qp->Execute();
	if (rs && !rs->IsEOF())
	{
	    unsigned int destfid = i->second;
	    std::string s = outputdir;
	    char outputleaf[40];
	    unsigned int drive = (drivesize[0] > drivesize[1]) ? 1 : 0;
	    sprintf(outputleaf, "/drive%u/_%05x/%03x", drive^1,
		    destfid/4096, destfid & 4095);
	    unlink((s+outputleaf).c_str());

	    sprintf(outputleaf, "/drive%u/_%05x/%03x", drive,
		    destfid/4096, destfid & 4095);
	    unlink((s+outputleaf).c_str());

	    util::MkdirParents((s+outputleaf).c_str());
	    
	    unsigned int type = rs->GetInteger(mediadb::TYPE);
	    unsigned int length;
	    if (type == mediadb::TUNE || type == mediadb::SPOKEN)
	    {
		unsigned int highfid = rs->GetInteger(mediadb::IDHIGH);
		if (highfid)
		{
		    qp = thedb->CreateQuery();
		    qp->Where(qp->Restrict(mediadb::ID, db::EQ, highfid));
		    db::RecordsetPtr rs2 = qp->Execute();
		    if (rs2 && !rs2->IsEOF())
			rs = rs2;
		    else
			TRACE << "Highfid " << highfid << " failed ("
			      << rs->GetString(mediadb::IDHIGH) << ")\n";
		}
		util::posix::MakeRelativeLink(s+outputleaf,
					      rs->GetString(mediadb::PATH));
		drivesize[drive] += rs->GetInteger(mediadb::SIZEBYTES);
		length = rs->GetInteger(mediadb::SIZEBYTES);
	    }
	    else
	    {
		std::vector<unsigned int> children;
		mediadb::ChildrenToVector(rs->GetString(mediadb::CHILDREN),
					  &children);

		length = 0;
		FILE *f = fopen((s+outputleaf).c_str(), "w");
		if (f)
		{
		    for (unsigned int j=0; j<children.size(); ++j)
		    {
			if (children[j])
			{
			    uint32_t lefid;
			    lefid = fidmap[children[j]];
			    if (lefid)
				length += (unsigned int)fwrite(&lefid, 1, 4, f);
			}
		    }
		    fclose(f);
		}
	    }

	    destfid |= 1;
	    sprintf(outputleaf, "/drive%u/_%05x/%03x", drive^1,
		    destfid/4096, destfid & 4095);
	    unlink((s+outputleaf).c_str());

	    sprintf(outputleaf, "/drive%u/_%05x/%03x", drive,
		    destfid/4096, destfid & 4095);
	    unlink((s+outputleaf).c_str());

	    FILE *f = fopen((s+outputleaf).c_str(), "w");
	    if (f)
	    {
		fprintf(f, "type=%s\n", typemap[type]);
		fprintf(f, "title=%s\n",
			rs->GetString(mediadb::TITLE).c_str());
		fprintf(f, "length=%u\n", length);
		if (type == mediadb::TUNE || type == mediadb::SPOKEN)
		{
		    unsigned int rate
			= rs->GetInteger(mediadb::BITSPERSEC)/1000;
		    std::string ebr =(boost::format("vs%u") % rate).str();
		    fprintf(f, "bitrate=%s\n", ebr.c_str());
		    fprintf(f, "codec=%s\n", 
			    codecmap[rs->GetInteger(mediadb::CODEC)]);
		    fprintf(f, "duration=%u\n",
			    rs->GetInteger(mediadb::DURATIONMS));
		    fprintf(f, "samplerate=%u\n",
			    rs->GetInteger(mediadb::SAMPLERATE));
		    MaybeWriteString(f, rs, "artist", mediadb::ARTIST);
		    MaybeWriteString(f, rs, "genre",  mediadb::GENRE);
		    MaybeWriteString(f, rs, "source", mediadb::ALBUM);
		    MaybeWriteInt(f, rs, "tracknr", mediadb::TRACKNUMBER);
		    MaybeWriteInt(f, rs, "year",    mediadb::YEAR);
		}

		fclose(f);
	    }
	}
    }

    printf("Drive 0: %llu bytes used\n", (unsigned long long)drivesize[0]);
    printf("Drive 1: %llu bytes used\n", (unsigned long long)drivesize[1]);
}

int main(int argc, char *argv[])
{
    static const struct option options[] =
    {
	{ "help",  no_argument, NULL, 'h' },
	{ "threads", required_argument, NULL, 't' },
	{ "dbfile", required_argument, NULL, 'f' },
	{ NULL, 0, NULL, 0 }
    };

    int nthreads = 0;
    const char *dbfile = DEFAULT_DB_FILE;

    int option_index;
    int option;
    while ((option = getopt_long(argc, argv, "ht:f:", options, &option_index))
	   != -1)
    {
	switch (option)
	{
	case 'h':
	    Usage(stdout);
	    return 0;
	case 't':
	    nthreads = (int)strtoul(optarg, NULL, 10);
	    break;
	case 'f':
	    dbfile = optarg;
	    break;
	default:
	    Usage(stderr);
	    return 1;
	}
    }

    int nargs = argc-optind;
    if (nargs < 2 || nargs > 3 || nthreads < 0)
    {
	Usage(stderr);
	return 1;
    }

    if (!nthreads)
	nthreads = util::CountCPUs() * 2;

    const char *outputdir = argv[optind];
    const char *mediaroot = argv[optind+1];
    const char *flacroot  = argv[optind+2];
    if (!flacroot)
	flacroot = "";

    db::steam::Database sdb(mediadb::FIELD_COUNT);
    sdb.SetFieldInfo(mediadb::ID, 
		     db::steam::FIELD_INT|db::steam::FIELD_INDEXED);
    sdb.SetFieldInfo(mediadb::PATH,
		     db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);

    TRACE << "reading\n";

    mediadb::ReadXML(&sdb, "db.xml");

    TRACE << "scanning\n";

    util::WorkerThreadPool wtp(util::WorkerThreadPool::NORMAL, nthreads);

    mediadb::LocalDatabase ldb(&sdb);

#ifdef HAVE_TAGLIB
    import::FileScanner ifs(mediaroot, flacroot, &ldb, &wtp);

    ifs.Scan();

    TRACE << "writing\n";

    FILE *f = fopen("db.xml", "wb");
    mediadb::WriteXML(&sdb, 1, f);
    fclose(f);

    TRACE << "arranging\n";

    WriteFIDStructure(outputdir, &sdb);
#endif

    return 0;
}
