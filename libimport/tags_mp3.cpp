#include "tags_mp3.h"
#include "libutil/trace.h"
#include "libmediadb/schema.h"
#include <mpegfile.h> /* from TagLib */
#include <id3v2tag.h>
#include <id3v1genres.h>
#include <textidentificationframe.h>
#include <errno.h>
#include <sys/stat.h>

namespace import {
namespace mp3 {

static const struct
{
    unsigned int field;
    const char *id3v2;
} tagmap[] = {
    { mediadb::TITLE,          "TIT2" },
    { mediadb::ARTIST,         "TPE1" },
    { mediadb::ALBUM,          "TALB" },
    { mediadb::TRACKNUMBER,    "TRCK" },

    { mediadb::GENRE,          "TCON" },
//    { mediadb::COMMENT,     "COMM" }, // NB special type
    { mediadb::YEAR,           "TYER" },

    { mediadb::MOOD,           "TMOO" },
    { mediadb::ORIGINALARTIST, "TOPE" },

    { mediadb::REMIXED,        "TPE4" },
    { mediadb::CONDUCTOR,      "TPE3" },
    { mediadb::COMPOSER,       "TCOM" },
    { mediadb::ENSEMBLE,       "TPE2" },
    { mediadb::LYRICIST,       "TEXT" },
};

enum { NUM_TAGS = sizeof(tagmap)/sizeof(tagmap[0]) };

unsigned Tags::Write(db::RecordsetPtr tags)
{
//    TRACE << "Opening '" << m_filename << "'\n";

    TagLib::MPEG::File tff(m_filename.c_str());
    TagLib::ID3v2::Tag *tag = tff.ID3v2Tag(true);

    for (unsigned int i=0; i<NUM_TAGS; ++i)
    {
	std::string s = tags->GetString(tagmap[i].field);
	if (!s.empty())
	{
	    bool full_unicode = false;
	    for (unsigned int j=0; j<s.length(); ++j)
	    {
		if ((unsigned char)(s[j]) > 127)
		{
		    full_unicode = true;
		    break;
		}
	    }

	    TagLib::String::Type enc = full_unicode ? TagLib::String::UTF16
		                                    : TagLib::String::Latin1;

	    TagLib::ID3v2::TextIdentificationFrame *tif
		= new TagLib::ID3v2::TextIdentificationFrame(tagmap[i].id3v2,
							     enc);
	    tif->setText(TagLib::String(s,TagLib::String::UTF8));

	    tag->addFrame(tif);
	}
    }
    
    tff.save(TagLib::MPEG::File::ID3v2, true);
    TRACE << "MP3 file tagged\n";
    return 0;
}

static std::string safe(const TagLib::String& s)
{
    if (s.isNull() || s.isEmpty())
	return std::string();
    return s.to8Bit(true);
}

unsigned Tags::Read(db::RecordsetPtr tags)
{
    TagLib::MPEG::File tff(m_filename.c_str());

    if (!tff.tag() || !tff.audioProperties())
    {
	// Not an audio file
	return EINVAL;
    }

    tags->SetInteger(mediadb::TYPE, mediadb::TUNE);
    tags->SetString(mediadb::PATH, m_filename);
    tags->SetInteger(mediadb::CODEC, mediadb::MP3);
//    tags->SetInteger(mediadb::OFFSET, tff.firstFrameOffset());

    struct stat st;
    if (stat(m_filename.c_str(), &st) == 0)
    {
	tags->SetInteger(mediadb::MTIME, st.st_mtime);
	tags->SetInteger(mediadb::CTIME, st.st_ctime);
    }
    
    boost::mutex::scoped_lock lock(s_taglib_mutex);

    TagLib::Tag *tag = tff.tag();
    tags->SetString(mediadb::TITLE, safe(tag->title()));
    tags->SetString(mediadb::ARTIST, safe(tag->artist()));
    tags->SetString(mediadb::ALBUM, safe(tag->album()));
    tags->SetString(mediadb::COMMENT, safe(tag->comment()));
    std::string genre = safe(tag->genre());
    int genreindex;
    if (sscanf(genre.c_str(), "(%d)", &genreindex) == 1)
    {
	genre = safe(TagLib::ID3v1::genre(genreindex));
    }
    tags->SetString(mediadb::GENRE, genre);
    tags->SetInteger(mediadb::YEAR, tag->year());
    tags->SetInteger(mediadb::TRACKNUMBER, tag->track());

    const TagLib::AudioProperties *ap = tff.audioProperties();
    tags->SetInteger(mediadb::DURATIONMS, ap->length()*1000);
    tags->SetInteger(mediadb::CHANNELS, ap->channels());
    tags->SetInteger(mediadb::BITSPERSEC, ap->bitrate()*1000);
    tags->SetInteger(mediadb::SAMPLERATE, ap->sampleRate());

    if (tff.ID3v2Tag())
    {
	const TagLib::ID3v2::FrameListMap& fm = tff.ID3v2Tag()->frameListMap();
	for (unsigned int i=0; i < NUM_TAGS; ++i)
	{
	    TagLib::ID3v2::FrameList fl = fm[tagmap[i].id3v2];
	    if (!fl.isEmpty())
	    {
		tags->SetString(tagmap[i].field, 
				safe(fl.front()->toString()));
	    }
	}
    }
    return 0;
}

}; // namespace mp3
}; // namespace import
