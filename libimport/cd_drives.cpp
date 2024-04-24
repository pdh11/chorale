#include "config.h"
#include "cd_drives.h"
#include "audio_cd.h"
#include "libutil/trace.h"
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <sstream>
#if HAVE_LIBCDIOP && !HAVE_PARANOIA
# include <cdio/cdio.h>
#else
# if HAVE_LINUX_CDROM_H
#  include <linux/cdrom.h>
# elif HAVE_SYS_DISK_H
#  include <sys/disk.h>
# endif
# include <sys/ioctl.h>
#endif
#include "libutil/task.h"
#include "libutil/worker_thread_pool.h"
#include "libutil/counted_pointer.h"
#if HAVE_WINDOWS_H
# include <windows.h>
#endif

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
#ifdef WIN32
    DWORD d = ::GetLogicalDrives();
    for (unsigned i=0; i<26; ++i)
    {
	if (d & (1<<i))
	{
	    char buf[4];
	    sprintf(buf, "%c:", 'A' + i);
	    UINT t = ::GetDriveTypeA(buf);
	    if (t == DRIVE_CDROM)
	    {
		if (!m_map.count(buf))
		    m_map[buf] = CDDrivePtr(new LocalCDDrive(buf));
	    }
	}
    }
#else
    {
	/* libcdio gets this wrong if /dev/cdrom is a symlink */
    
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
#endif
}

CDDrivePtr CDDrives::GetDriveForDevice(const std::string& device)
{
    if (m_map.count(device))
	return m_map[device];
    return CDDrivePtr();
}


        /* LocalCDDrive::Impl */


class LocalCDDrive::Impl
{
    std::string m_device;
    util::WorkerThreadPool m_threads; // One per CD drive
    std::string m_volume_udi;

public:
    explicit Impl(const std::string& device)
	: m_device(device),
	  m_threads(util::WorkerThreadPool::NORMAL, 1)
    {
    }

    ~Impl()
    {
    }

    std::string GetDevice() const { return m_device; }
    util::TaskQueue *GetTaskQueue() { return &m_threads; }

    bool DiscPresent() const { return !m_volume_udi.empty(); }
};


        /* LocalCDDrive itself */


LocalCDDrive::LocalCDDrive(const std::string& device)
    : m_impl(new Impl(device))
{
}

LocalCDDrive::~LocalCDDrive()
{
    delete m_impl;
}

bool LocalCDDrive::SupportsDiscPresent() const
{
    return false;
}

bool LocalCDDrive::DiscPresent() const
{
    return m_impl->DiscPresent();
}

std::string LocalCDDrive::GetName() const
{
    return m_impl->GetDevice();
}

std::string LocalCDDrive::GetDevice() const
{
    return m_impl->GetDevice();
}

util::TaskQueue *LocalCDDrive::GetTaskQueue()
{
    return m_impl->GetTaskQueue();
}

unsigned int LocalCDDrive::Eject()
{
    TRACE << "Ejecting " << GetDevice() << "\n";

#if HAVE_LIBCDIOP && !HAVE_PARANOIA
    cdio_eject_media_drive(GetDevice().c_str());
    return 0;
#else
    int fd = ::open(GetDevice().c_str(), O_RDONLY|O_NONBLOCK);
    if (fd < 0)
    {
	TRACE << "Can't open device: " << errno << "\n";
	return (unsigned int)errno;
    }

#if HAVE_LINUX_CDROM_H
    int lock = 0;
    (void)::ioctl(fd, CDROM_LOCKDOOR, lock); // might fail
    int rc = ::ioctl(fd, CDROMSTOP);
    if (rc < 0) {
        TRACE << "Can't stop CD: " << errno << "\n";
    } else {
        rc = ::ioctl(fd, CDROMEJECT);
        if (rc < 0) {
            TRACE << "Can't eject CD: " << errno << "\n";
        }
    }
#elif HAVE_SYS_DISK_H
    int rc = ::ioctl(fd, DKIOCEJECT); /* MacOS */
#endif
    ::close(fd);

    return (rc < 0) ? (unsigned int)errno : 0;
#endif
}

unsigned int LocalCDDrive::GetCD(AudioCDPtr *result)
{
    return LocalAudioCD::Create(GetDevice(), result);
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
