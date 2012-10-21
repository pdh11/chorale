#ifndef IMPORT_PLAYLIST_H
#define IMPORT_PLAYLIST_H

#include "libutil/choose_by_extension.h"
#include <string>
#include <list>

namespace import {

class PlaylistIO;

class Playlist
{
    util::ChooseByExtension<PlaylistIO> m_chooser;

public:
    Playlist();
    ~Playlist();

    unsigned int Init(const std::string& filename);

    unsigned int Load(std::list<std::string> *entries);
    unsigned int Save(const std::list<std::string> *entries);
};

} // namespace import

#endif
