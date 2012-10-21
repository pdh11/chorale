#include "config.h"
#include "playlist_wpl.h"
#include "libutil/trace.h"
#include "libutil/file.h"
#include "libutil/xmlescape.h"

#ifdef HAVE_LIBXMLPP

#include <libxml++/parsers/saxparser.h>
#include <errno.h>

namespace import {

class PlaylistWPLParser: public xmlpp::SaxParser
{
    Playlist *m_playlist;
    size_t m_index;

public:
    explicit PlaylistWPLParser(Playlist *playlist)
	: m_playlist(playlist),
	  m_index(0)
	{}

    void on_start_element(const Glib::ustring& name,
			  const AttributeList& properties)
	{
	    if (name == "media")
	    {
		for (AttributeList::const_iterator i = properties.begin();
		     i != properties.end();
		     ++i)
		{
		    if (i->name == "src")
		    {
			m_playlist->SetEntry(m_index++,
					     util::MakeAbsolutePath(
						 m_playlist->GetFilename(),
						 i->value));
		    }
		}
	    }
	}
};

unsigned int PlaylistWPL::Load()
{
    try
    {
	PlaylistWPLParser parser(this);
	parser.set_substitute_entities(true);
	parser.parse_file(GetFilename());
    }
    catch (...)
    {
    }
    return 0;
}

unsigned int PlaylistWPL::Save()
{
    FILE *f = fopen(GetFilename().c_str(), "w+");
    if (!f)
    {
	TRACE << "failed " << errno << "\n";
	return (unsigned)errno;
    }

    fprintf(f, "<?wpl version=\"1.0\"?>\n<smil>\n<body>\n<seq>\n");

    for (size_t i = 0; i < GetLength(); ++i)
    {
	std::string relpath = util::MakeRelativePath(GetFilename(),
						     GetEntry(i));
	fprintf(f, "<media src=\"%s\"/>\n",
		util::XmlEscape(relpath).c_str());
    }
    fprintf(f, "</seq>\n</body>\n</smil>\n");
    unsigned int rc = ferror(f) ? (unsigned)errno : 0u;
    fclose(f);
    return rc;
}

} // namespace import

#endif // HAVE_LIBXMLPP

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
    std::string wplname = util::Canonicalise("test.wpl");
    import::PlaylistPtr pp = import::Playlist::Create(wplname);

    for (unsigned int i=0; i<TESTS; ++i)
    {
	pp->SetEntry(i, util::MakeAbsolutePath(wplname, tests[i]));
    }
    unsigned int rc = pp->Save();
    assert(rc == 0);

    pp = NULL;
    pp = import::Playlist::Create(wplname);
    rc = pp->Load();
    assert(rc == 0);

    assert(pp->GetLength() == TESTS);

    for (unsigned int i=0; i<TESTS; ++i)
    {
	assert(pp->GetEntry(i) == util::MakeAbsolutePath(wplname, tests[i]));
    }

    pp = NULL;
    
    unlink(wplname.c_str());

    return 0;
}

#endif
