#include "eject_task.h"
#include "cd_drive.h"

namespace import {

unsigned int EjectTask::Run(void)
{
    return m_cd->Eject();
}

} // namespace import
