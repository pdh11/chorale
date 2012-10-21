#ifndef EJECT_TASK_H
#define EJECT_TASK_H

#include "libutil/task.h"
#include <string>
#include "cd_drives.h"

namespace import {

/** Not much of a task, but encapsulated as one so it gets queued after all
 * the ripping.
 */
class EjectTask: public util::Task
{
    CDDrivePtr m_cd;
    explicit EjectTask(CDDrivePtr cd) : m_cd(cd) {}

public:
    static util::TaskPtr Create(CDDrivePtr cd)
    {
	return util::TaskPtr(new EjectTask(cd));
    }

    unsigned int Run();
};

} // namespace import

#endif
