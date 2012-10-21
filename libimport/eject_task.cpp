#include "eject_task.h"

namespace import {

void EjectTask::Run(void)
{
    m_cd->Eject();
}

} // namespace import
