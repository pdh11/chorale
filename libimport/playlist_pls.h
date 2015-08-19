#ifndef IMPORT_PLAYLIST_PLS_H
#define IMPORT_PLAYLIST_PLS_H

#include <string>
#include "playlist_io.h"

namespace import {

class PlaylistPLS: public PlaylistIO
{
public:
    unsigned int Load(const std::string& filename,
		      std::list<std::string> *entries) override;
    unsigned int Save(const std::string& filename,
		      const std::list<std::string> *entries) override;
};

} // namespace import

#endif
