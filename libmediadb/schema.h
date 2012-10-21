/* mediadb/schema.h
 *
 * Database schema for media files
 */
#ifndef MEDIADB_SCHEMA_H
#define MEDIADB_SCHEMA_H 1

#include <vector>
#include <string>

#undef CTIME

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
    AUDIOCODEC,

    SIZEBYTES, // 10
    BITSPERSEC,
    SAMPLERATE,
    CHANNELS,
    PATH, // Interpretation depends on value of TYPE

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

    CHILDREN, ///< {nchildren, child-id, child-id...} as UTF-8 string
    IDHIGH,   ///< High-quality (FLAC) version of the file, or 0
    IDPARENT, ///< (An arbitrary one of) the parent directories of this file
    VIDEOCODEC,
    CONTAINER, ///< i.e. file format

    FIELD_COUNT
};

enum { SCHEMA_VERSION = 1 };

/** Special values of ID field */
enum {
    BROWSE_ROOT = 0x100,
    RADIO_ROOT  =  0xf0,
    EPG_ROOT    =  0xe0,
    TV_ROOT     =  0xd0
};

/** Values of TYPE field
 *
 * TYPE=TUNE/TUNEHIGH/SPOKEN/RADIO has an AUDIO_CODEC tag
 * TYPE=IMAGE has a CONTAINER tag
 * TYPE=VIDEO/TV has AUDIOCODEC, VIDEOCODEC and CONTAINER tags
 */
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
    TV,
    QUERY,   ///< Not found in actual databases, but used by libmediatree

    PENDING, ///< Placeholder for a future recording

    TYPE_COUNT
};

/** Values of AUDIOCODEC field */
enum {
    NONE = 0,
    MP2,
    MP3,
    FLAC,
    VORBIS,
    WAV,
    PCM,
    AAC,
    WMA,

    AUDIOCODEC_COUNT
};

/** Values of VIDEOCODEC field */
enum {
    /* NONE */
    MPEG2 = 1,
    MPEG4,   /* MPEG-4 part 2 */
    H264,    /* MPEG-4 part 10, AVC */
    FLV,
    WMV,
    THEORA,

    VIDEOCODEC_COUNT
};

/** Values of CONTAINER field */
enum {
    /* NONE */
    OGG = 1,
    MATROSKA,
    AVI, 
    MPEGPS, /* MPEG-1 or -2 program stream */
    MP4,    /* MPEG-4 part 14 */
    MOV,    /* Quicktime */
    JPEG,   /* and/or JFIF */
    
    CONTAINER_COUNT
};

/** Turn the internal representation of a "children" field into a vector.
 */
void ChildrenToVector(const std::string& children_field,
		      std::vector<unsigned int> *vec_out);

std::string VectorToChildren(const std::vector<unsigned int>& vec_in);

} // namespace mediadb

#endif
