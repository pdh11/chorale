#include "config.h"
#include "cd_drives.h"
#include "local_cd_drive.h"
#include "audio_cd.h"
#include "libutil/trace.h"
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <sstream>
#if HAVE_LINUX_CDROM_H
# include <linux/cdrom.h>
#elif HAVE_SYS_DISK_H
# include <sys/disk.h>
#endif
#include <sys/ioctl.h>
#include "libutil/worker_thread_pool.h"
#include "libutil/counted_pointer.h"

namespace import {


        /* CDDrives */


CDDrives::CDDrives()
{
}

CDDrives::~CDDrives()
{
}

void CDDrives::Refresh()
{
    std::ifstream f("/proc/sys/dev/cdrom/info");

    while (!f.eof() && !f.fail())
    {
        std::string line;
        std::getline(f, line);

        if (std::string(line, 0, 11) == "drive name:")
        {
            std::istringstream is(line);
            std::string token;
            is >> token;
            is >> token;

            while (!is.eof())
            {
                token.clear();
                is >> token;
                if (!token.empty())
                {
                    token = "/dev/" + token;
                    if (!m_map.count(token))
                        m_map[token] = CDDrivePtr(new LocalCDDrive(token));
                }
            }
            break;
        }
    }
}

CDDrivePtr CDDrives::GetDriveForDevice(const std::string& device)
{
    if (m_map.count(device))
	return m_map[device];
    return CDDrivePtr();
}

} // namespace import

#ifdef TEST

int main()
{
    import::CDDrives cdd;
    cdd.Refresh();
    return 0;
}

#endif
