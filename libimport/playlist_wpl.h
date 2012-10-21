#ifndef IMPORT_PLAYLIST_WPL_H
#define IMPORT_PLAYLIST_WPL_H

#include "playlist.h"

namespace import {

class PlaylistWPL: public Playlist
{
public:
    // Being a Playlist
    unsigned int Load();
    unsigned int Save();

    /** Parser callback when an entry is found */
    unsigned int OnSrc(const std::string&);
};

} // namespace import

#endif
