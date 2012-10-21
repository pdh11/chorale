#include "cd_toc_task.h"

namespace import {

CDTocTaskPtr CDTocTask::Create(CDDrivePtr drive, CDDBService *cddb)
{
    return CDTocTaskPtr(new CDTocTask(drive, cddb));
}

/** Not much of a task but it needs doing on a background thread.
 */
void CDTocTask::Run()
{
    if (m_cddb)
    {
	FireProgress(1,3);
	m_drive->GetCD(&m_cd);
	FireProgress(2,3);
	if (m_cd)
	    m_lookup = m_cddb->Lookup(m_cd);
	FireProgress(3,3);
    }
    else
    {
	FireProgress(1,2);
	m_drive->GetCD(&m_cd);
	FireProgress(2,2);
    }
}

} // namespace import
