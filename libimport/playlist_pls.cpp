#include "playlist_pls.h"
#include "libutil/file_stream.h"
#include "libutil/file.h"
#include "libutil/utf8.h"
#include "libutil/trace.h"
#include "libutil/http.h"
#include "libutil/counted_pointer.h"
#include "libutil/printf.h"
#include <sstream>
#include <locale.h>
#include <string.h>
#include <unistd.h>

namespace import {

unsigned int PlaylistPLS::Load(const std::string& filename,
			       std::list<std::string> *entries)
{
    std::unique_ptr<util::Stream> ss;
    unsigned int rc = util::OpenFileStream(filename.c_str(), util::READ, &ss);
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
                TRACE << "Got " << ptr+1 << "\n";
		if (util::http::IsHttpURL(ptr+1))
		{
		    entries->push_back(ptr+1);
		}
		else
		{
		    entries->push_back(util::MakeAbsolutePath(filename,
							      ptr+1));
		}
	    }
	}

    } while (!s.empty());

    return 0;
}

unsigned int PlaylistPLS::Save(const std::string& filename,
			       const std::list<std::string> *entries)
{
    std::unique_ptr<util::Stream> ss;
    unsigned int rc = util::OpenFileStream(filename.c_str(), util::WRITE, &ss);
    
    if (rc)
	return rc;

    rc = ss->WriteString("[playlist]\n");
    if (!rc)
	rc = ss->WriteString(util::Printf() << "numberofentries="
			     << entries->size() << "\n");
    if (rc)
	return rc;

    unsigned int n=0;
    for (std::list<std::string>::const_iterator i = entries->begin();
	 i != entries->end();
	 ++i)
    {
	std::string s = util::UTF8ToLocalEncoding(i->c_str());

	if (!util::http::IsHttpURL(s)) {
	    s = util::MakeRelativePath(filename, s);
        }
	
	rc = ss->WriteString(util::Printf() << "File" << ++n
			     << "=" << s << "\n");
	if (rc) {
	    return rc;
        }

        TRACE << "wrote " << s << "\n";
    }

    return 0;
}

} // namespace import

#ifdef TEST

# include "playlist.h"
# include "libutil/http.h"

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
    std::string name = util::Canonicalise("test.pls");
    import::Playlist pp;

    unsigned int rc = pp.Init(name);
    assert(rc == 0);

    std::list<std::string> entries;

    for (unsigned int i=0; i<TESTS; ++i)
    {
	if (util::http::IsHttpURL(tests[i]))
	    entries.push_back(tests[i]);
	else
	    entries.push_back(util::MakeAbsolutePath(name, tests[i]));
    }
    rc = pp.Save(&entries);
    assert(rc == 0);

    entries.clear();

    rc = pp.Load(&entries);
    assert(rc == 0);

    TRACE << "sz=" << entries.size() << " n=" << TESTS << "\n";

    assert(entries.size() == TESTS);

    for (unsigned int i=0; i<TESTS; ++i)
    {
	if (!strncmp(tests[i], "http://", 7))
	{
	    assert(entries.front() == tests[i]);
	}
	else
	{
	    assert(entries.front() == util::MakeAbsolutePath(name, tests[i]));
	}
	entries.pop_front();
    }
    
    unlink(name.c_str());
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
