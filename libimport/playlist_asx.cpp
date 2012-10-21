#include "config.h"
#include "playlist_asx.h"
#include "libutil/trace.h"
#include "libutil/file.h"
#include "libutil/xmlescape.h"

#ifdef HAVE_LIBXMLPP

#include <libxml++/libxml++.h>
#include <errno.h>

namespace import {

class PlaylistASXParser: public xmlpp::SaxParser
{
    Playlist *m_playlist;
    size_t m_index;

public:
    explicit PlaylistASXParser(Playlist *playlist)
	: m_playlist(playlist),
	  m_index(0)
	{}

    void on_start_element(const Glib::ustring& name,
			  const AttributeList& properties)
	{
	    if (name == "ref")
	    {
		for (AttributeList::const_iterator i = properties.begin();
		     i != properties.end();
		     ++i)
		{
		    if (i->name == "href")
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

unsigned int PlaylistASX::Load()
{
    try
    {
	PlaylistASXParser parser(this);
	parser.set_substitute_entities(true);
	parser.parse_file(GetFilename());
    }
    catch (...)
    {
    }
    return 0;
}

unsigned int PlaylistASX::Save()
{
    FILE *f = fopen(GetFilename().c_str(), "w+");
    if (!f)
    {
	TRACE << "failed " << errno << "\n";
	return errno;
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

#endif // HAVE_LIBXMLPP
