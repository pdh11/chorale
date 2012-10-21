#include "eject_task.h"
#include "libutil/trace.h"
#include <cdio/cdio.h>

namespace import {

void EjectTask::Run(void)
{
    cdio_eject_media_drive(m_cd->GetDevice().c_str());
}

}; // namespace import
