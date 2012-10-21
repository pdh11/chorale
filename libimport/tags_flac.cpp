#include "config.h"
#include "tags_flac.h"
#include "vorbis_comment.h"
#include "libutil/trace.h"
#include "libdb/recordset.h"
#include "libmediadb/schema.h"

#if HAVE_TAGLIB

#include <flacfile.h> /* from TagLib */
#include <xiphcomment.h>

namespace import {
namespace flac {

unsigned Tags::Write(db::RecordsetPtr tags)
{
//    TRACE << "Opening '" << m_filename << "'\n";
	    
    typedef std::map<std::string, std::string> tagmap_t;
    tagmap_t tagmap;
    for (unsigned int i=0; i<mediadb::FIELD_COUNT; ++i)
    {
	const char *s = import::GetVorbisTagForField(i);
	if (s)
	{
	    std::string val = tags->GetString(i);
	    if (!val.empty())
		tagmap[s] = val;
	}
    }

    TagLib::FLAC::File tff(m_filename.c_str());
    TagLib::Ogg::XiphComment *oxc = tff.xiphComment(true);
    for (tagmap_t::const_iterator i = tagmap.begin(); i != tagmap.end(); ++i)
    {
	if (!i->second.empty())
	    oxc->addField(i->first, TagLib::String(i->second, 
						   TagLib::String::UTF8));
    }
    tff.save();
    TRACE << "FLAC file tagged\n";
    return 0;
}

} // namespace flac
} // namespace import

#endif // HAVE_TAGLIB
