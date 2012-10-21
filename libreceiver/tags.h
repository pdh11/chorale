#ifndef RECEIVER_TAGS_H
#define RECEIVER_TAGS_H

/** Classes for implementing Rio Receiver protocol support.
 *
 * That is, functionality common to both servers (receiverd) and clients
 * (db::receiver).
 */
namespace receiver {

enum {
    FID,
    TITLE,
    ARTIST,
    SOURCE,
    TRACKNR,

    GENRE,
    YEAR,
    DURATION,
    CODEC,
    LENGTH,
    
    BITRATE,
    SAMPLERATE,
    TYPE,
    MOOD,
    ORIGINALARTIST,

    REMIXED,
    CONDUCTOR,
    COMPOSER,
    ENSEMBLE,
    LYRICIST,

    TAG_COUNT
};

/** Given a value from the above enum, what's its name? */
const char *NameForTag(int receivertag);

/** Given a tag name, what's its mediadb::TAG number? */
int TagForName(const char *tag);

} // namespace receiver

#endif
