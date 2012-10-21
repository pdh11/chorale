#ifndef IMPORT_PLAYLIST_ASX_H
#define IMPORT_PLAYLIST_ASX_H

#include <string>
#include "playlist_io.h"

namespace import {

class PlaylistASX: public PlaylistIO
{
public:
    unsigned int Load(const std::string& filename,
		      std::list<std::string> *entries);
    unsigned int Save(const std::string& filename,
		      const std::list<std::string> *entries);
};

} // namespace import

#endif
