#include "libutil/trace.h"
#include "cd_drives.h"
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include "libutil/task.h"
#include "libutil/worker_thread.h"

namespace import {


        /* CDDrives */


CDDrives::CDDrives()
{
}

void CDDrives::Refresh()
{
    /* libcdio gets this wrong if /dev/cdrom is a symlink */
    
    std::ifstream f("/proc/sys/dev/cdrom/info");

    while (!f.eof())
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
			m_map[token] = CDDrivePtr(new CDDrive(token));
		}
	    }
	    break;
	}
    }

//    m_map["/dev/fcd0"] = CDDrivePtr(new CDDrive("/dev/fcd0"));
}

CDDrivePtr CDDrives::GetDriveForDevice(const std::string& device)
{
    if (m_map.count(device))
	return m_map[device];
    return CDDrivePtr();
}


        /* CDDrive */


class CDDrive::CDDriveImpl
{
    std::string m_device;
    util::TaskQueue m_queue;
    util::WorkerThread m_thread; // One per CD drive

public:
    explicit CDDriveImpl(const std::string& device)
	: m_device(device), m_thread(&m_queue)
    {}
    ~CDDriveImpl()
    {
	m_queue.PushTask(util::TaskPtr());
    }

    std::string GetDevice() const { return m_device; }
    util::TaskQueue *GetTaskQueue() { return &m_queue; }
};

CDDrive::CDDrive(const std::string& device)
    : m_impl(new CDDriveImpl(device))
{
}

CDDrive::~CDDrive()
{
    delete m_impl;
}

std::string CDDrive::GetDevice() const
{
    return m_impl->GetDevice();
}

util::TaskQueue *CDDrive::GetTaskQueue()
{
    return m_impl->GetTaskQueue();
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
