#ifndef EJECT_TASK_H
#define EJECT_TASK_H

#include "libutil/task.h"
#include "libutil/task_queue.h"
#include <string>
#include "libutil/counted_pointer.h"
#include "libutil/bind.h"

namespace import {

class CDDrive;

/** Not much of a task, but encapsulated as one so it gets queued after all
 * the ripping.
 */
class EjectTask: public util::Task
{
    util::CountedPointer<CDDrive> m_cd;
    explicit EjectTask(util::CountedPointer<CDDrive> cd) : util::Task("eject"), m_cd(cd) {}

    unsigned int Run();

    typedef util::CountedPointer<EjectTask> EjectTaskPtr;

public:

    static util::TaskCallback Create(util::CountedPointer<CDDrive> cd)
    {
	EjectTaskPtr ejp(new EjectTask(cd));
	return util::Bind(ejp).To<&EjectTask::Run>();
    }
};

} // namespace import

#endif
