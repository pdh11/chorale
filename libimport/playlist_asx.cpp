#include "playlist_asx.h"
#include "libutil/trace.h"
#include "libutil/file.h"
#include "libutil/file_stream.h"
#include "libutil/xmlescape.h"
#include "libutil/xml.h"
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

namespace import {

extern const char ASX[] = "asx";
extern const char ENTRY[] = "entry";
extern const char REF[] = "ref";
extern const char HREF[] = "href";


typedef xml::Parser<
    xml::Tag<ASX,
	     xml::Tag<ENTRY,
		      xml::Tag<REF,
			       xml::Attribute<HREF,
					      PlaylistXMLObserver,
					      &PlaylistXMLObserver::OnHref
					      > > > > > ASXParser;

unsigned int PlaylistASX::Load(const std::string& filename,
			       std::list<std::string> *entries)
{
    PlaylistXMLObserver obs(filename, entries);
    std::auto_ptr<util::Stream> ss;
    unsigned int rc = util::OpenFileStream(filename.c_str(), util::READ, &ss);
    if (rc != 0)
	return rc;

    ASXParser parser;
    return parser.Parse(ss.get(), &obs);
}

unsigned int PlaylistASX::Save(const std::string& filename,
			       const std::list<std::string> *entries)
{
    FILE *f = fopen(filename.c_str(), "w+");
    if (!f)
    {
	TRACE << "failed " << errno << "\n";
	return (unsigned)errno;
    }

    fprintf(f, "<asx version=\"3.0\">\n");

    for (std::list<std::string>::const_iterator i = entries->begin();
	 i != entries->end();
	 ++i)
    {
	std::string relpath = util::MakeRelativePath(filename, *i);
	fprintf(f, "<entry><ref href=\"%s\"/></entry>\n",
		util::XmlEscape(relpath).c_str());
    }
    fprintf(f, "</asx>\n");
    fclose(f);
    return 0;
}

} // namespace import

#ifdef TEST

# include "playlist.h"

static const char *const tests[] = {
    "sibling.mp3",
    "foo/child.mp3",
    "../aunt.mp3",
    "X&Y.mp3",
    "X<>Y.mp3",
    "Sigur R\xc3\xb3s.mp3"
};

enum { TESTS = sizeof(tests)/sizeof(tests[0]) };

int main()
{
    std::string name = util::Canonicalise("test.asx");
    import::Playlist pp;

    unsigned int rc = pp.Init(name);
    assert(rc == 0);

    std::list<std::string> entries;

    for (unsigned int i=0; i<TESTS; ++i)
    {
	entries.push_back(util::MakeAbsolutePath(name, tests[i]));
    }
    rc = pp.Save(&entries);
    assert(rc == 0);

    entries.clear();

    rc = pp.Load(&entries);
    assert(rc == 0);

    assert(entries.size() == TESTS);

    for (unsigned int i=0; i<TESTS; ++i)
    {
	assert(entries.front() == util::MakeAbsolutePath(name, tests[i]));
	entries.pop_front();
    }

    unlink(name.c_str());

    return 0;
}

#endif
