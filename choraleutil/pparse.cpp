#include "libimport/playlist.h"
#include <stdio.h>

int main(int /*argc*/, char *argv[])
{
    for (char **pp = argv+1; *pp; ++pp)
    {
	printf("%s\n", *pp);
	import::PlaylistPtr pls = import::Playlist::Create(*pp);
	if (pls)
	{
	    pls->Load();
	    for (size_t i = 0; i < pls->GetLength(); ++i)
	    {
		printf("  %s\n", pls->GetEntry(i).c_str());
	    }
	}
    }
    return 0;
}
