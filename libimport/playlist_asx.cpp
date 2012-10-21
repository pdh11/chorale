#include "playlist_asx.h"
#include "libutil/trace.h"
#include "libutil/file.h"
#include "libutil/file_stream.h"
#include "libutil/xmlescape.h"
#include "libutil/xml.h"
#include <errno.h>
#include <string.h>

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
					      PlaylistASX,
					      &PlaylistASX::OnHref
					      > > > > > ASXParser;

unsigned int PlaylistASX::OnHref(const std::string& value)
{
    AppendEntry(util::MakeAbsolutePath(GetFilename(), value));
    return 0;
}

unsigned int PlaylistASX::Load()
{
    util::SeekableStreamPtr ss;
    unsigned int rc = util::OpenFileStream(GetFilename().c_str(), util::READ,
					   &ss);
    if (rc != 0)
	return rc;

    ASXParser parser;
    return parser.Parse(ss, this);
}

unsigned int PlaylistASX::Save()
{
    /// @bug Filename is UTF-8 on Windows

    FILE *f = fopen(GetFilename().c_str(), "w+");
    if (!f)
    {
	TRACE << "failed " << errno << "\n";
	return (unsigned)errno;
    }

    fprintf(f, "<asx version=\"3.0\">\n");

    for (size_t i = 0; i < GetLength(); ++i)
    {
	std::string relpath = util::MakeRelativePath(GetFilename(),
						     GetEntry(i));
	fprintf(f, "<entry><ref href=\"%s\"/></entry>\n",
		util::XmlEscape(relpath).c_str());
    }
    fprintf(f, "</asx>\n");
    fclose(f);
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
    "Sigur R\xc3\xb3s.mp3"
};

enum { TESTS = sizeof(tests)/sizeof(tests[0]) };

int main()
{
    std::string asxname = util::Canonicalise("test.asx");
    import::PlaylistPtr pp = import::Playlist::Create(asxname);

    for (unsigned int i=0; i<TESTS; ++i)
    {
	pp->SetEntry(i, util::MakeAbsolutePath(asxname, tests[i]));
    }
    unsigned int rc = pp->Save();
    assert(rc == 0);

    pp = NULL;
    pp = import::Playlist::Create(asxname);
    rc = pp->Load();
    assert(rc == 0);

    assert(pp->GetLength() == TESTS);

    for (unsigned int i=0; i<TESTS; ++i)
    {
	assert(pp->GetEntry(i) == util::MakeAbsolutePath(asxname, tests[i]));
    }

    pp = NULL;
    
    unlink(asxname.c_str());

    return 0;
}

#endif
