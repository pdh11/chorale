#include "vorbis_comment.h"
#include "libmediadb/schema.h"
#include <stdlib.h>

namespace import {

static const struct {
    unsigned field;
    const char *vorbistag;
} fieldmap[] = {
    { mediadb::ALBUM,          "album" },
    { mediadb::ARTIST,         "artist" },
    { mediadb::COMMENT,        "comment" },
    { mediadb::COMPOSER,       "composer" },
    { mediadb::CONDUCTOR,      "conductor" },
    { mediadb::YEAR,           "date" },
    { mediadb::ENSEMBLE,       "ensemble" },
    { mediadb::GENRE,          "genre" },
    { mediadb::LYRICIST,       "lyricist" },
    { mediadb::MOOD,           "mood" },
    { mediadb::ORIGINALARTIST, "originalartist" },
    { mediadb::REMIXED,        "remixed" },
    { mediadb::TITLE,          "title" },
    { mediadb::TRACKNUMBER,    "tracknumber" },
};
 
enum { NUM_FIELDS = sizeof(fieldmap)/sizeof(fieldmap[0]) };
 
const char *GetVorbisTagForField(unsigned field)
{
    for (unsigned i=0; i<NUM_FIELDS; ++i)
    {
	if (fieldmap[i].field == field)
	    return fieldmap[i].vorbistag;
    }
    return NULL;
}

} // namespace import
