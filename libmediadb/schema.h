/* mediadb/schema.h
 *
 * Database schema for media files
 */
#ifndef MEDIADB_SCHEMA_H
#define MEDIADB_SCHEMA_H 1

#include <vector>
#include <string>

namespace mediadb {

enum {
    ID = 0,
    TITLE,
    ARTIST,
    ALBUM,
    TRACKNUMBER,

    GENRE,
    COMMENT,
    YEAR,
    DURATIONMS,
    CODEC,

    SIZEBYTES, // 10
    BITSPERSEC,
    SAMPLERATE,
    CHANNELS,
    PATH,

    MTIME,
    CTIME,
    TYPE,
    MOOD,
    ORIGINALARTIST,

    REMIXED, // 20
    CONDUCTOR,
    COMPOSER,
    ENSEMBLE,
    LYRICIST,

    CHILDREN, ///< {#children, child-id, child-id...} as UTF-8 string
    IDHIGH,   ///< High-quality (FLAC) version of the file, or 0
    IDPARENT, ///< (An arbitrary one of) the parent directories of this file

    FIELD_COUNT
};

enum { SCHEMA_VERSION = 1 };

/** Values of TYPE field */
enum {
    FILE = 0, ///< i.e. not one of the other types of file
    TUNE,
    TUNEHIGH, ///< FLAC version of a tune also available as MP3
    SPOKEN,
    PLAYLIST,
    DIR,
    IMAGE,
    VIDEO,
    RADIO,

    TYPE_COUNT
};

/** Values of CODEC field */
enum {
    NONE = 0,
    MP2,
    MP3,
    FLAC,
    OGGVORBIS,
    WAV,
    JPEG,

    CODEC_COUNT
};

/** Turn the internal representation of a "children" field into a vector.
 */
void ChildrenToVector(const std::string& children_field,
		      std::vector<unsigned int> *vec_out);

std::string VectorToChildren(const std::vector<unsigned int>& vec_in);

}; // namespace mediadb

#endif
