#ifndef CD_TOC_TASK_H
#define CD_TOC_TASK_H 1

#include "libutil/task.h"
#include "cddb_service.h"
#include "libutil/counted_pointer.h"

namespace import {

class CDDrive;
class AudioCD;

class CDTocTask: public util::Task
{
    util::CountedPointer<CDDrive> m_drive;
    util::CountedPointer<AudioCD> m_cd;
    CDDBService *m_cddb;
    CDDBLookupPtr m_lookup;

    CDTocTask(util::CountedPointer<CDDrive> drive, CDDBService *cddb)
	: m_drive(drive), m_cddb(cddb) {}

public:
    typedef util::CountedPointer<CDTocTask> CDTocTaskPtr;

    static CDTocTaskPtr Create(util::CountedPointer<CDDrive> cd,
                               CDDBService *cddb);

    unsigned int Run();

    bool IsValid() { return m_cd; }
    util::CountedPointer<AudioCD> GetCD() { return m_cd; }
    CDDBLookupPtr GetCDDB() { return m_lookup; }
};

typedef util::CountedPointer<CDTocTask> CDTocTaskPtr;

} // namespace import

#endif
