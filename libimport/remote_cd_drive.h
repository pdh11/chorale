#ifndef LIBIMPORT_REMOTE_CD_DRIVE_H
#define LIBIMPORT_REMOTE_CD_DRIVE_H 1

#include "cd_drive.h"
#include "audio_cd.h"
#include "libupnp/OpticalDrive_client.h"
#include "libutil/worker_thread_pool.h"

namespace util { class TaskQueue; }
namespace util { class Scheduler; }

namespace import {

class RemoteCDDrive: public CDDrive, public upnp::OpticalDriveObserver
{
    util::http::Client *m_client;
    upnp::DeviceClient m_device_client;
    upnp::OpticalDriveClient m_optical_drive;
    std::string m_friendly_name;
    bool m_disc_present;

    /** Our own thread.
     *
     * @todo This totally isn't necessary, it's really just here to serialise
     *       ripping tasks. Fix by (a) making each ripping task start the next
     *       one (b) having each local CD be a BufferSource so it can serve
     *       ripping and playback simultaneously.
     */
    util::WorkerThreadPool m_threads;

public:
    RemoteCDDrive(util::http::Client*, util::http::Server*, 
		  util::Scheduler*);
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
    util::http::Client *m_client;

    friend class RemoteCDDrive;

public:
    ~RemoteAudioCD();

    // Being an AudioCD
    std::unique_ptr<util::Stream> GetTrackStream(unsigned int track);
};

} // namespace import

#endif
