#ifndef IMPORT_PLAYLIST_ASX_H
#define IMPORT_PLAYLIST_ASX_H

#include "playlist.h"

namespace import {

class PlaylistASX: public Playlist
{
public:
    // Being a Playlist
    unsigned int Load();
    unsigned int Save();
};

};

#endif
