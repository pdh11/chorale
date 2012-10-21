#ifndef EJECT_TASK_H
#define EJECT_TASK_H

#include "libutil/task.h"
#include <string>
#include "cd_drives.h"
#include "libutil/counted_pointer.h"

namespace import {

/** Not much of a task, but encapsulated as one so it gets queued after all
 * the ripping.
 */
class EjectTask: public util::Task
{
    CDDrivePtr m_cd;
    explicit EjectTask(CDDrivePtr cd) : m_cd(cd) {}

    unsigned int Run();

    typedef util::CountedPointer<EjectTask> EjectTaskPtr;

public:
    static util::TaskCallback Create(CDDrivePtr cd)
    {
	EjectTaskPtr ejp(new EjectTask(cd));
	return util::Bind(ejp).To<&EjectTask::Run>();
    }
};

} // namespace import

#endif
