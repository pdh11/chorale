#include "config.h"
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
}

unsigned int LocalCDDrive::GetCD(AudioCDPtr *result)
{
    return LocalAudioCD::Create(GetDevice(), result);
}

} // namespace import
