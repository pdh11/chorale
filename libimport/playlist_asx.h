#ifndef IMPORT_PLAYLIST_ASX_H
#define IMPORT_PLAYLIST_ASX_H

#include <string>
#include "playlist.h"

namespace import {

class PlaylistASX: public Playlist
{
public:
    // Being a Playlist
    unsigned int Load();
    unsigned int Save();

    /** Parser callback when an entry is found */
    unsigned int OnHref(const std::string&);
};

} // namespace import

#endif
