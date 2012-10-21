#include "eject_task.h"
#include "libutil/trace.h"
#include <unistd.h>
#include <fcntl.h>
#include <linux/cdrom.h>
#include <sys/ioctl.h>

namespace import {

void EjectTask::Run(void)
{
    int fd = ::open(m_cd->GetDevice().c_str(), O_RDONLY|O_NONBLOCK);
    ::ioctl(fd, CDROMEJECT);
    ::close(fd);
}

}; // namespace import
