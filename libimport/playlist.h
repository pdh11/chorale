#ifndef IMPORT_PLAYLIST_H
#define IMPORT_PLAYLIST_H

#include "libutil/counted_object.h"
#include <string>

namespace import {

class Playlist: public util::CountedObject<>
{
    class Impl;
    Impl *m_impl;
    
protected:
    Playlist();

public:
    virtual ~Playlist();
    size_t GetLength() const;
    std::string GetEntry(size_t index) const; // 0-based, returns full path
    std::string GetFilename() const;
    
    void SetEntry(size_t index, const std::string& fullpath); // 0-based
    void AppendEntry(const std::string& fullpath);

    virtual unsigned int Load() = 0;
    virtual unsigned int Save() = 0;
    typedef boost::intrusive_ptr<Playlist> PlaylistPtr;

    static PlaylistPtr Create(const std::string& filename);
};

typedef boost::intrusive_ptr<Playlist> PlaylistPtr;

} // namespace import

#endif
