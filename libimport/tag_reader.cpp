#include "tag_reader.h"
#include "config.h"
#include "tags_mutex.h"
#include "libdb/recordset.h"
#include "libutil/file.h"
#include "libmediadb/schema.h"
#include <errno.h>

#undef CTIME

#if HAVE_TAGLIB

#include <fileref.h>
#include <tag.h>

namespace import {

unsigned TagReaderBase::Read(const std::string& filename, db::Recordset *rs)
{
    util::Mutex::Lock lock(s_taglib_mutex);

    TagLib::FileRef fr(filename.c_str());

    if (!fr.tag() || !fr.audioProperties())
	return EINVAL;

    rs->SetInteger(mediadb::TYPE, mediadb::TUNE);
    rs->SetString(mediadb::PATH, filename);

    /// @todo do this properly
    if (util::GetExtension(filename.c_str()) == "flac")
	rs->SetInteger(mediadb::AUDIOCODEC, mediadb::FLAC);

    struct stat st;
    if (stat(filename.c_str(), &st) == 0)
    {
	rs->SetInteger(mediadb::MTIME, (unsigned int)st.st_mtime);
	rs->SetInteger(mediadb::CTIME, (unsigned int)st.st_ctime);
	rs->SetInteger(mediadb::SIZEBYTES, (unsigned int)st.st_size);
    }

    const TagLib::Tag *tag = fr.tag();    
    rs->SetString(mediadb::TITLE, tag->title().to8Bit(true));
    rs->SetString(mediadb::ARTIST, tag->artist().to8Bit(true));
    rs->SetString(mediadb::ALBUM, tag->album().to8Bit(true));
    rs->SetString(mediadb::COMMENT, tag->comment().to8Bit(true));
    rs->SetString(mediadb::GENRE, tag->genre().to8Bit(true));
    rs->SetInteger(mediadb::YEAR, tag->year());
    rs->SetInteger(mediadb::TRACKNUMBER, tag->track());

    const TagLib::AudioProperties *ap = fr.audioProperties();
    rs->SetInteger(mediadb::DURATIONMS, ap->length()*1000);
    rs->SetInteger(mediadb::CHANNELS, ap->channels());
    rs->SetInteger(mediadb::BITSPERSEC, ap->bitrate()*1000);
    rs->SetInteger(mediadb::SAMPLERATE, ap->sampleRate());

    if (rs->GetString(mediadb::TITLE).empty())
	rs->SetString(mediadb::TITLE,
		      util::StripExtension(
			  util::GetLeafName(filename.c_str()).c_str()
			  ));

    return 0;
}

util::Mutex s_taglib_mutex;

} // namespace import

#endif // HAVE_TAGLIB
