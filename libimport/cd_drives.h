#ifndef CD_DRIVES_H
#define CD_DRIVES_H

#include <map>
#include <string>
#include "libutil/counted_pointer.h"

namespace import {

class CDDrive;

class CDDrives
{
    typedef std::map<std::string, util::CountedPointer<CDDrive> > map_t;
    map_t m_map;

public:
    CDDrives();
    ~CDDrives();

    void Refresh();

    typedef map_t::const_iterator const_iterator;
    const_iterator begin() { return m_map.begin(); }
    const_iterator end() { return m_map.end(); }

    util::CountedPointer<CDDrive> GetDriveForDevice(const std::string& device);
};

} // namespace import

#endif
