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

	import::PlaylistPtr pls = import::Playlist::Create(canon);
	if (pls)
	{
	    pls->Load();
	    for (size_t i = 0; i < pls->GetLength(); ++i)
	    {
		canon = util::Canonicalise(pls->GetEntry(i));
		printf("  %s\n", canon.c_str());
	    }
	}
    }
    return 0;
}
