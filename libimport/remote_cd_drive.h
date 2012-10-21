#ifndef LIBIMPORT_REMOTE_CD_DRIVE_H
#define LIBIMPORT_REMOTE_CD_DRIVE_H 1

#include "cd_drives.h"
#include "libupnp/OpticalDrive_client.h"
#include "libutil/task.h"
#include "libutil/worker_thread.h"

namespace import {

class RemoteCDDrive: public CDDrive, public upnp::OpticalDriveObserver
{
    upnp::Client m_upnp;
    upnp::OpticalDriveClient m_optical_drive;
    std::string m_friendly_name;
    bool m_disc_present;
    util::TaskQueue m_queue;
    util::WorkerThread m_thread; // One per CD drive

public:
    RemoteCDDrive();
    ~RemoteCDDrive();

    unsigned Init(const std::string& url, const std::string& udn);
    
    // Being a CDDrive
    std::string GetName() const;
    bool SupportsDiscPresent() const;
    bool DiscPresent() const;
    unsigned int Eject();
    util::TaskQueue *GetTaskQueue();
    unsigned int GetCD(AudioCDPtr *result);

    // Being a upnp::OpticalDriveObserver
    void OnDiscPresent(bool);
};

class RemoteAudioCD: public AudioCD
{
    std::vector<std::string> m_urls;

    friend class RemoteCDDrive;

public:
    ~RemoteAudioCD();

    // Being an AudioCD
    util::SeekableStreamPtr GetTrackStream(unsigned int track);
};

} // namespace import

#endif