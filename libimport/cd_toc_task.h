#ifndef CD_TOC_TASK_H
#define CD_TOC_TASK_H 1

#include "libutil/task.h"
#include "cd_drives.h"
#include "audio_cd.h"
#include "cddb_service.h"

namespace import {

class CDTocTask: public util::Task
{
    CDDrivePtr m_drive;
    AudioCDPtr m_cd;
    CDDBService *m_cddb;
    CDDBLookupPtr m_lookup;

    CDTocTask(CDDrivePtr drive, CDDBService *cddb) 
	: m_drive(drive), m_cddb(cddb) {}

public:
    typedef boost::intrusive_ptr<CDTocTask> CDTocTaskPtr;

    static CDTocTaskPtr Create(CDDrivePtr cd, CDDBService *cddb);

    virtual void Run();

    bool IsValid() { return m_cd; }
    AudioCDPtr GetCD() { return m_cd; }
    CDDBLookupPtr GetCDDB() { return m_lookup; }
};

typedef boost::intrusive_ptr<CDTocTask> CDTocTaskPtr;

} // namespace import

#endif
