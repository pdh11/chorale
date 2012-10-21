#include "playlist_wpl.h"
#include "libutil/trace.h"
#include "libutil/file.h"
#include "libutil/file_stream.h"
#include "libutil/xmlescape.h"
#include "libutil/xml.h"
#include <errno.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

namespace import {

extern const char SMIL[] = "smil";
extern const char BODY[] = "body";
extern const char SEQ[] = "seq";
extern const char MEDIA[] = "media";
extern const char SRC[] = "src";

typedef xml::Parser<
    xml::Tag<SMIL,
	     xml::Tag<BODY,
		      xml::Tag<SEQ,
			       xml::Tag<MEDIA,
					xml::Attribute<SRC,
						       PlaylistXMLObserver,
						       &PlaylistXMLObserver::OnHref
						       > > > > > > WPLParser;

unsigned int PlaylistWPL::Load(const std::string& filename,
			       std::list<std::string> *entries)
{
    PlaylistXMLObserver obs(filename, entries);
    std::auto_ptr<util::Stream> ss;
    unsigned int rc = util::OpenFileStream(filename.c_str(), util::READ, &ss);
    if (rc != 0)
	return rc;

    WPLParser parser;
    return parser.Parse(ss.get(), &obs);
}

unsigned int PlaylistWPL::Save(const std::string& filename,
			       const std::list<std::string> *entries)
{
    FILE *f = fopen(filename.c_str(), "w+");
    if (!f)
    {
	TRACE << "failed " << errno << "\n";
	return (unsigned)errno;
    }

    fprintf(f, "<?wpl version=\"1.0\"?>\n<smil>\n<body>\n<seq>\n");

    for (std::list<std::string>::const_iterator i = entries->begin();
	 i != entries->end();
	 ++i)
    {
	std::string relpath = util::MakeRelativePath(filename, *i);
	fprintf(f, "<media src=\"%s\"/>\n",
		util::XmlEscape(relpath).c_str());
    }
    fprintf(f, "</seq>\n</body>\n</smil>\n");
    unsigned int rc = ferror(f) ? (unsigned)errno : 0u;
    fclose(f);
    return rc;
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
    std::string name = util::Canonicalise("test.wpl");
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
