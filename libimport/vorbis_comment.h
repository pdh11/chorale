#ifndef VORBIS_COMMENT_H
#define VORBIS_COMMENT_H 1

namespace import {

/** Returns the Vorbiscomment tag corresponding to the mediadb.h field (or
 * NULL if none).
 */
const char *GetVorbisTagForField(unsigned field);

} // namespace import

#endif
