#include "tags.h"
#include "tags_flac.h"
#include "tags_mp3.h"
#include "libutil/file.h"
#include <errno.h>
#include "libmediadb/schema.h"
#include <sys/stat.h>
#include <fileref.h>
#include <tag.h>

namespace import {

TagsPtr Tags::Create(const std::string& filename)
{
    std::string ext = util::GetExtension(filename.c_str());

    if (ext == "mp3")
	return TagsPtr(new mp3::Tags(filename));

    if (ext == "flac")
	return TagsPtr(new flac::Tags(filename));

    return TagsPtr(new Tags(filename));
}

unsigned Tags::Read(db::RecordsetPtr rs)
{
    TagLib::FileRef fr(m_filename.c_str());

    if (!fr.tag() || !fr.audioProperties())
	return EINVAL;

    rs->SetInteger(mediadb::TYPE, mediadb::TUNE);
    rs->SetString(mediadb::PATH, m_filename);

    // @todo do this properly
    if (util::GetExtension(m_filename.c_str()) == "flac")
	rs->SetInteger(mediadb::CODEC, mediadb::FLAC);

    struct stat st;
    if (stat(m_filename.c_str(), &st) == 0)
    {
	rs->SetInteger(mediadb::MTIME, st.st_mtime);
	rs->SetInteger(mediadb::CTIME, st.st_ctime);
	rs->SetInteger(mediadb::SIZEBYTES, st.st_size);
    }

    boost::mutex::scoped_lock lock(s_taglib_mutex);

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

    return 0;
}

unsigned Tags::Write(db::RecordsetPtr)
{
    // Default is to give up
    return EINVAL;
}

unsigned WriteTags(const std::string& filename, db::RecordsetPtr tags)
{
    TagsPtr tp = Tags::Create(filename);
    return tp->Write(tags);
}

boost::mutex s_taglib_mutex;

}; // namespace import
