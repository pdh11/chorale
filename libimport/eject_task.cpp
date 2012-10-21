#include "eject_task.h"

namespace import {

unsigned int EjectTask::Run(void)
{
    return m_cd->Eject();
}

} // namespace import
