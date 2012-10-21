/* cd_drives.h
 */

#ifndef CD_DRIVES_H
#define CD_DRIVES_H

#include <map>
#include <string>
#include "libutil/counted_object.h"

namespace util { class TaskQueue; }

namespace import {

class CDDrives;

class CDDrive: public CountedObject
{
    class CDDriveImpl;
    CDDriveImpl *m_impl;

    explicit CDDrive(const std::string& device);

    friend class CDDrives;

public:
    ~CDDrive();

    std::string GetDevice() const;
    bool IsBusy() const;
    void SetBusy(bool busy);

    util::TaskQueue *GetTaskQueue();
};

typedef boost::intrusive_ptr<CDDrive> CDDrivePtr;

class CDDrives
{
    typedef std::map<std::string, CDDrivePtr> map_t;

    map_t m_map;

public:
    CDDrives();

    void Refresh();

    typedef map_t::const_iterator const_iterator;
    const_iterator begin() { return m_map.begin(); }
    const_iterator end() { return m_map.end(); }

    CDDrivePtr GetDriveForDevice(const std::string& device);
};

} // namespace import

#endif
