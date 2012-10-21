#ifndef IMPORT_PLAYLIST_PLS_H
#define IMPORT_PLAYLIST_PLS_H

#include <string>
#include "playlist.h"

namespace import {

class PlaylistPLS: public Playlist
{
public:
    // Being a Playlist
    unsigned int Load();
    unsigned int Save();
};

} // namespace import

#endif
