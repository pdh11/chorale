#ifndef LIBIMPORT_PLAYLIST_IO_H
#define LIBIMPORT_PLAYLIST_IO_H

#include <string>
#include <list>

namespace import {

class PlaylistIO
{
public:
    virtual ~PlaylistIO() {}

    virtual unsigned int Load(const std::string& filename,
			      std::list<std::string> *entries) = 0;
    virtual unsigned int Save(const std::string& filename,
			      const std::list<std::string> *entries) = 0;
};

/** For playlists that involve parsing XML
 */
class PlaylistXMLObserver
{
    const std::string& m_filename;
    std::list<std::string> *m_entries;

public:
    PlaylistXMLObserver(const std::string& filename, 
			std::list<std::string> *entries);
    unsigned int OnHref(const std::string& value);
};

} // namespace import

#endif
