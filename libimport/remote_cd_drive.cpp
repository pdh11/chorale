#include "remote_cd_drive.h"
#include "libupnp/description.h"
#include "libutil/ssdp.h"
#include "libutil/http_stream.h"
#include "libutil/trace.h"

namespace import {

RemoteCDDrive::RemoteCDDrive()
    : m_disc_present(true),
      m_thread(&m_queue)
{
}

RemoteCDDrive::~RemoteCDDrive()
{
    m_queue.PushTask(util::TaskPtr());
}

unsigned RemoteCDDrive::Init(const std::string& url, const std::string& udn)
{
    unsigned int rc = m_upnp.Init(url, udn);
    if (rc != 0)
	return rc;

    m_friendly_name = m_upnp.GetDescription().GetFriendlyName();

    rc = m_optical_drive.Init(&m_upnp, util::ssdp::s_uuid_opticaldrive);
    if (rc != 0)
	return rc;

    m_optical_drive.AddObserver(this);
    return 0;
}

std::string RemoteCDDrive::GetName() const
{
    return m_friendly_name;
}

bool RemoteCDDrive::SupportsDiscPresent() const
{
    return true;
}

bool RemoteCDDrive::DiscPresent() const
{
    return m_disc_present;
}

unsigned int RemoteCDDrive::Eject()
{
    return m_optical_drive.Eject();
}

util::TaskQueue *RemoteCDDrive::GetTaskQueue()
{
    return &m_queue;
}

void RemoteCDDrive::OnDiscPresent(bool whether)
{
    m_disc_present = whether;
    Fire(&CDDriveObserver::OnDiscPresent, whether);
}

unsigned int RemoteCDDrive::GetCD(AudioCDPtr *result)
{
    uint8_t tracks;
    int32_t sectors;
    unsigned int rc = m_optical_drive.GetDiscInfo(&tracks, &sectors);
    if (rc != 0)
    {
	TRACE << "GetDiscInfo failed: " << rc << "\n";
	return rc;
    }
    RemoteAudioCD *cd = new RemoteAudioCD;
    cd->m_urls.resize(tracks);
    cd->m_toc.resize(tracks);
    cd->m_total_sectors = sectors;
    
    for (uint8_t i=0; i<tracks; ++i)
    {
	upnp::OpticalDrive::TrackType type;
	rc = m_optical_drive.GetTrackInfo(i, &type,
					  &cd->m_toc[i].first_sector,
					  &cd->m_toc[i].last_sector,
					  &cd->m_urls[i]);
	if (rc != 0)
	{
	    TRACE << "GetTrackInfo(" << i << ") failed: " << rc << "\n";
	    delete cd;
	    return rc;
	}
    }
    *result = AudioCDPtr(cd);
    return 0;
}

RemoteAudioCD::~RemoteAudioCD()
{
}

util::SeekableStreamPtr RemoteAudioCD::GetTrackStream(unsigned int track)
{
    if (track >= m_urls.size())
	return util::SeekableStreamPtr();
    util::HTTPStreamPtr ptr;
    unsigned int rc = util::HTTPStream::Create(&ptr, m_urls[track].c_str());
    if (rc != 0)
    {
	TRACE << "Can't create HTTP stream: " << rc << "\n";
	return util::SeekableStreamPtr();
    }
    return ptr;
}

} // namespace import
