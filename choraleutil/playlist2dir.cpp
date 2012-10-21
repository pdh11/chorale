#include "config.h"
#include "libimport/playlist.h"
#include "libutil/file.h"
#include "libutil/counted_pointer.h"
#include <getopt.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sstream>
#include <iomanip>

void Usage(FILE *f)
{
    fprintf(f,
	 "Usage: playlist2dir <playlist.asx>\n\n"
"    Converts a playlist file (currently only '.asx' playlists) to a directory\n"
"of symbolic links.\n"
"    Built on " __DATE__ ".\n"
	);
}

int main(int argc, char *argv[])
{
    static const struct option options[] =
    {
	{ "help", no_argument, NULL, 'h' },
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
	    return 1;
	}
    }

    if (optind != argc-1)
    {
	Usage(stderr);
	return 1;
    }

    import::Playlist pp;
    std::list<std::string> entries;
    if (pp.Init(argv[optind])
	|| pp.Load(&entries))
    {
	fprintf(stderr, "Can't load playlist file '%s'\n", argv[optind]);
	return 1;
    }

    std::string dirname = util::StripExtension(argv[optind]);
    util::Mkdir(dirname.c_str());

    unsigned int n=0;
    for (std::list<std::string>::const_iterator i = entries.begin();
	 i != entries.end();
	 ++i)
    {
	std::ostringstream linkname;
	std::string target = *i;
	linkname << dirname << "/" << std::setw(3) << std::setfill('0')
		 << (++n) << " " << util::GetLeafName(target.c_str());
	util::posix::MakeRelativeLink(linkname.str(), target);
    }
    return 0;
}
