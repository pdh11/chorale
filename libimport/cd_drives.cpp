#include "libutil/trace.h"
#include "cd_drives.h"
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <sstream>
//#include <linux/cdrom.h>
//#include <sys/ioctl.h>
#include <cdio/cdio.h>
#include "libutil/task.h"
#include "libutil/worker_thread.h"

namespace import {


        /* CDDrives */


CDDrives::CDDrives(util::hal::Context *hal)
    : m_hal(hal)
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
    if (m_hal)
    {
	m_hal->GetMatchingDevices("storage.drive_type", "cdrom", this);
    }
    else
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
			    m_map[token] = CDDrivePtr(new LocalCDDrive(token));
		    }
		}
		break;
	    }
	}
    }
#endif
}

void CDDrives::OnDevice(util::hal::DevicePtr dev)
{
#ifndef WIN32
    std::string device = dev->GetString("block.device");
    if (!m_map.count(device))
	m_map[device] = CDDrivePtr(new LocalCDDrive(device, m_hal));
#endif
}

CDDrivePtr CDDrives::GetDriveForDevice(const std::string& device)
{
    if (m_map.count(device))
	return m_map[device];
    return CDDrivePtr();
}


        /* LocalCDDrive::Impl */


class LocalCDDrive::Impl: public util::hal::Observer,
			  public util::hal::DeviceObserver
{
    LocalCDDrive *m_parent;
    std::string m_device;
    util::hal::Context *m_hal;
    util::TaskQueue m_queue;
    util::WorkerThread m_thread; // One per CD drive
    std::string m_volume_udi;

public:
    Impl(LocalCDDrive *parent, const std::string& device,
	 util::hal::Context *hal)
	: m_parent(parent),
	  m_device(device), 
	  m_hal(hal), 
	  m_thread(&m_queue)
    {
#ifndef WIN32
	if (m_hal)
	{
	    m_hal->AddObserver(this);
	    m_hal->GetMatchingDevices("volume.disc.type", "cd_rom", this);
	}
#endif
    }

    ~Impl()
    {
#ifndef WIN32
	if (m_hal)
	    m_hal->RemoveObserver(this);
#endif
	m_queue.PushTask(util::TaskPtr());
    }

    std::string GetDevice() const { return m_device; }
    util::TaskQueue *GetTaskQueue() { return &m_queue; }

    bool SupportsDiscPresent() const { return m_hal != NULL; }
    bool DiscPresent() const { return !m_volume_udi.empty(); }

    // Being a util::hal::Observer
    void OnDeviceAdded(util::hal::DevicePtr);
    void OnDeviceRemoved(util::hal::DevicePtr);

    // Being a util::hal::DeviceObserver
    void OnDevice(util::hal::DevicePtr);
};

void LocalCDDrive::Impl::OnDeviceAdded(util::hal::DevicePtr dev)
{
#ifndef WIN32
    if (dev->GetString("block.device") == m_device
	&& dev->GetString("volume.disc.type") == "cd_rom")
    {
	m_volume_udi = dev->GetUDI();
//	TRACE << "CD present\n";
	m_parent->Fire(&CDDriveObserver::OnDiscPresent, true);
    }
#endif
}

void LocalCDDrive::Impl::OnDeviceRemoved(util::hal::DevicePtr dev)
{
#ifndef WIN32
    if (dev->GetUDI() == m_volume_udi)
    {
//	TRACE << "CD no longer present\n";
	m_parent->Fire(&CDDriveObserver::OnDiscPresent, false);
	m_volume_udi.clear();
    }
#endif
}

void LocalCDDrive::Impl::OnDevice(util::hal::DevicePtr dev)
{
#ifndef WIN32
    /* This is called with all CD-ROM volumes, we need only check if the block
     * device is us.
     */
    if (dev->GetString("block.device") == m_device)
	m_volume_udi = dev->GetUDI();
#endif
}


        /* LocalCDDrive itself */


LocalCDDrive::LocalCDDrive(const std::string& device, util::hal::Context *hal)
    : m_impl(new Impl(this, device, hal))
{
}

LocalCDDrive::~LocalCDDrive()
{
    delete m_impl;
}

bool LocalCDDrive::SupportsDiscPresent() const
{
    return m_impl->SupportsDiscPresent();
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

#if 1
    cdio_eject_media_drive(GetDevice().c_str());
    return 0;
#else
    int fd = ::open(GetDevice().c_str(), O_RDONLY|O_NONBLOCK);
    if (fd < 0)
    {
	TRACE << "Can't open device: " << errno << "\n";
	return (unsigned int)errno;
    }

    int rc = ::ioctl(fd, CDROMEJECT);
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
