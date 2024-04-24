/* cd_drives.h
 */

#ifndef CD_DRIVES_H
#define CD_DRIVES_H

#include <map>
#include <string>
#include "libutil/counted_object.h"
#include "libutil/counted_pointer.h"
#include "libutil/observable.h"

namespace util { class TaskQueue; }

namespace import {

class AudioCD;

class CDDrives;

class CDDriveObserver
{
public:
    virtual ~CDDriveObserver() {}
    virtual void OnDiscPresent(bool) = 0;
};

class CDDrive: public util::CountedObject,
	       public util::Observable<CDDriveObserver>
{
public:
    virtual ~CDDrive() {}

    virtual std::string GetName() const = 0;

    virtual bool SupportsDiscPresent() const = 0;
    virtual bool DiscPresent() const = 0;
    virtual unsigned int Eject() = 0;
    virtual util::TaskQueue *GetTaskQueue() = 0;

    /** Attempt to create an AudioCDPtr. Can take several seconds (CD spinup).
     * Fails if no CD, or if no audio tracks.
     */
    virtual unsigned int GetCD(util::CountedPointer<AudioCD> *result) = 0;
};

typedef util::CountedPointer<CDDrive> CDDrivePtr;

class LocalCDDrive: public CDDrive
{
    class Impl;
    Impl *m_impl;

    friend class Impl;

    LocalCDDrive(const std::string& device);

    friend class CDDrives;

public:
    ~LocalCDDrive();
    std::string GetDevice() const;

    // Being a CDDrive
    std::string GetName() const override;
    bool SupportsDiscPresent() const override;
    bool DiscPresent() const override;
    unsigned int Eject() override;
    util::TaskQueue *GetTaskQueue() override;
    unsigned int GetCD(util::CountedPointer<AudioCD> *result) override;
};

class CDDrives
{
    typedef std::map<std::string, CDDrivePtr> map_t;
    map_t m_map;

public:
    CDDrives();
    ~CDDrives();

    void Refresh();

    typedef map_t::const_iterator const_iterator;
    const_iterator begin() { return m_map.begin(); }
    const_iterator end() { return m_map.end(); }

    CDDrivePtr GetDriveForDevice(const std::string& device);
};

} // namespace import

#endif
