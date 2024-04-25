#ifndef CD_DRIVE_H
#define CD_DRIVE_H

#include <map>
#include <string>
#include "libutil/counted_object.h"
#include "libutil/counted_pointer.h"
#include "libutil/observable.h"

namespace util { class TaskQueue; }

namespace import {

class AudioCD;

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

} // namespace import

#endif
