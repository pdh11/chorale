#include "libimport/playlist.h"
#include "libutil/file.h"
#include "libutil/counted_pointer.h"
#include <stdio.h>

int main(int /*argc*/, char *argv[])
{
    for (char **pp = argv+1; *pp; ++pp)
    {
	printf("%s\n", *pp);
	std::string canon = util::Canonicalise(*pp);

	import::Playlist pls;
	std::list<std::string> entries;
	if (pls.Init(canon) == 0
	    && pls.Load(&entries) == 0)
	{
	    for (std::list<std::string>::const_iterator i = entries.begin();
		 i != entries.end();
		 ++i)
	    {
		canon = util::Canonicalise(*i);
		printf("  %s\n", canon.c_str());
	    }
	}
    }
    return 0;
}
