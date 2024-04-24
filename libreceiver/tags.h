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

} // namespace receiver

#endif
