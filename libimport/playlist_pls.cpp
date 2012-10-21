#include "playlist_pls.h"
#include "libutil/file_stream.h"
#include "libutil/file.h"
#include "libutil/utf8.h"
#include "libutil/trace.h"
#include "libutil/http_fetcher.h"
#include "libutil/counted_pointer.h"
#include <boost/format.hpp>
#include <locale.h>
#include <string.h>

namespace import {

unsigned int PlaylistPLS::Load()
{
    util::SeekableStreamPtr ss;
    unsigned int rc = util::OpenFileStream(GetFilename().c_str(), util::READ,
					   &ss);
    if (rc)
	return rc;

    std::string s;

    do {
	rc = ss->ReadLine(&s);
	if (rc)
	    return rc;

	s = util::LocalEncodingToUTF8(s.c_str());

	const char *ptr = s.c_str();
	if (!strncmp(ptr, "File", 4))
	{
	    ptr = strchr(ptr, '=');
	    if (ptr && ptr[1])
	    {
		if (util::http::IsHttpURL(ptr+1))
		{
		    AppendEntry(ptr+1);
		}
		else
		{
		    AppendEntry(util::MakeAbsolutePath(GetFilename(), ptr+1));
		}
	    }
	}

    } while (!s.empty());

    return 0;
}

unsigned int PlaylistPLS::Save()
{
    util::SeekableStreamPtr ss;
    unsigned int rc = util::OpenFileStream(GetFilename().c_str(), util::WRITE,
					   &ss);
    
    if (rc)
	return rc;

    rc = ss->WriteString("[playlist]\n");
    if (!rc)
	rc = ss->WriteString((boost::format("numberofentries=%u\n")
			     % GetLength()).str());
    if (rc)
	return rc;

    for (unsigned int i=0; i < GetLength(); ++i)
    {
	std::string s = util::UTF8ToLocalEncoding(GetEntry(i).c_str());

	if (!util::http::IsHttpURL(s))
	    s = util::MakeRelativePath(GetFilename(), s);

	rc = ss->WriteString((boost::format("File%u=%s\n")
			      % (i+1) % s).str());
	if (rc)
	    return rc;
    }

    return 0;
}

} // namespace import

#ifdef TEST

static const char *const tests[] = {
    "sibling.mp3",
    "foo/child.mp3",
    "../aunt.mp3",
    "X&Y.mp3",
    "X<>Y.mp3",
    "Sigur R\xc3\xb3s.mp3",
    "http://streamer-dtc-aa05.somafm.com:80/stream/1021"
};

enum { TESTS = sizeof(tests)/sizeof(tests[0]) };

void DoAllTests()
{
//    const char *loc = setlocale(LC_ALL, NULL);
//    if (loc)
//	TRACE << "Locale: " << loc << "\n";

    std::string plsname = util::Canonicalise("test.pls");
    import::PlaylistPtr pp = import::Playlist::Create(plsname);

    for (unsigned int i=0; i<TESTS; ++i)
    {
	if (!strncmp(tests[i], "http://", 7))
	    pp->SetEntry(i, tests[i]);
	else
	    pp->SetEntry(i, util::MakeAbsolutePath(plsname, tests[i]));
    }
    unsigned int rc = pp->Save();
    assert(rc == 0);

    pp = NULL;
    pp = import::Playlist::Create(plsname);
    rc = pp->Load();
    assert(rc == 0);

    assert(pp->GetLength() == TESTS);

    for (unsigned int i=0; i<TESTS; ++i)
    {
	if (!strncmp(tests[i], "http://", 7))
	{
	    assert(pp->GetEntry(i) == tests[i]);
	}
	else
	{
	    assert(pp->GetEntry(i) == util::MakeAbsolutePath(plsname, 
							     tests[i]));
	}
    }

    pp = NULL;
    
    unlink(plsname.c_str());
}

int main()
{
    setlocale(LC_ALL, "C");
    DoAllTests();
    setlocale(LC_ALL, "");
    DoAllTests();
    setlocale(LC_ALL, "en_GB.iso88591");
    DoAllTests();
    setlocale(LC_ALL, "en_GB.utf8");
    DoAllTests();
    return 0;
}

#endif
