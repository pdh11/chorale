#include "ripping_control.h"
#include "ripping_task.h"
#include "encoding_task_flac.h"
#include "encoding_task_mp3.h"
#include "eject_task.h"
#include "playlist.h"
#include "libdb/free_rs.h"
#include "libutil/file.h"
#include "libutil/trace.h"
#include "libutil/formatter.h"
#include "libmediadb/schema.h"

namespace import {

unsigned RippingControl::sm_index = 0;

struct RippingControl::Track
{
    RippingTaskPtr rtp;
    EncodingTaskPtr etp_flac;
    EncodingTaskPtr etp_mp3;
    db::RecordsetPtr tags;
};

RippingControl::RippingControl(import::CDDrivePtr drive, import::AudioCDPtr cd,
			       util::TaskQueue *cpu_queue, 
			       util::TaskQueue *disk_queue,
			       const std::string& mp3_root,
			       const std::string& flac_root)
    : m_cpu_queue(cpu_queue),
      m_disk_queue(disk_queue),
      m_mp3_root(mp3_root),
      m_flac_root(flac_root),
      m_ntracks((unsigned int)cd->GetTrackCount()),
      m_tracks(new Track[m_ntracks])
{
    unsigned int this_index = ++sm_index;

    for (unsigned int i=0; i<m_ntracks; ++i)
    {
	m_tracks[i].tags = db::FreeRecordset::Create();
    }

    for (unsigned int i=0; i<m_ntracks; ++i)
    {
	char leaf[32];
	sprintf(leaf, "cd%03ut%02u", this_index, i+1);
	std::string filename = m_flac_root + "/" + leaf;

	import::EncodingTaskPtr etp1
	    = import::EncodingTaskFlac::Create(filename + ".flac");
	m_tracks[i].etp_flac = etp1;
	etp1->SetObserver(this);

	filename = m_mp3_root + "/" + leaf;

	import::EncodingTaskPtr etp2
	    = import::EncodingTaskMP3::Create(filename + ".mp3");
	m_tracks[i].etp_mp3 = etp2;
	etp2->SetObserver(this);

	import::RippingTaskPtr rtp = 
	    import::RippingTask::Create(cd, i, filename, etp1, etp2,
					m_cpu_queue,
					m_disk_queue);
	m_tracks[i].rtp = rtp;
	rtp->SetObserver(this);
	drive->GetTaskQueue()->PushTask(rtp);
	TRACE << "Pushed task for track " << i << "/" << m_ntracks << "\n";
    }
    drive->GetTaskQueue()->PushTask(import::EjectTask::Create(drive));
    TRACE << "Pushed eject task\n";
}

RippingControl::~RippingControl()
{
    for (unsigned int i = 0; i < m_ntracks; ++i)
    {
	m_tracks[i].etp_flac->SetObserver(NULL);
	m_tracks[i].etp_mp3->SetObserver(NULL);
	if (m_tracks[i].rtp)
	    m_tracks[i].rtp->SetObserver(NULL);
    }
    delete[] m_tracks;
}

void RippingControl::OnProgress(const util::Task *task, unsigned num,
				unsigned denom)
{
    unsigned int type = PROGRESS_NONE;
    for (unsigned int i=0; i<m_ntracks; ++i)
    {
	if (task == m_tracks[i].etp_flac.get())
	    type = PROGRESS_FLAC;
	else if (task == m_tracks[i].etp_mp3.get())
	    type = PROGRESS_MP3;
	else if (task == m_tracks[i].rtp.get())
	{
	    type = PROGRESS_RIP;
	    if (num == denom)
	    {
		// Abandon references so the object is deleted
		m_tracks[i].rtp->SetObserver(NULL);
		m_tracks[i].rtp = NULL;
	    }
	}

	if (type != PROGRESS_NONE)
	{
	    Fire(&RippingControlObserver::OnProgress, i, type, num, denom);
	    return;
	}
    }
}

db::RecordsetPtr RippingControl::Tags(unsigned int track)
{
    if (track >= m_ntracks)
	return db::RecordsetPtr();
    return m_tracks[track].tags;
}

static std::string UnThe(const std::string& s)
{
    if (std::string(s,0,4) == "The ")
	return std::string(s,4);
    return s;
}

unsigned int RippingControl::Done()
{
    // Cheat by using first track's details
    std::string album_artist = m_tracks[0].tags->GetString(mediadb::ARTIST);
    std::string album_title = m_tracks[0].tags->GetString(mediadb::ALBUM);
    std::string album_year = m_tracks[0].tags->GetString(mediadb::YEAR);

    // Filename (playlist or dir) representing entire album
    std::string album_filename;

    import::PlaylistPtr pp;
    if (!m_playlist_template.empty())
    {
	util::Formatter formatter;

	formatter.AddField('a', util::ProtectLeafname(album_artist));
	formatter.AddField('A', util::ProtectLeafname(UnThe(album_artist)));
	formatter.AddField('c', util::ProtectLeafname(album_title));
	formatter.AddField('y', util::ProtectLeafname(album_year));
	
	std::string playlist_filename = m_mp3_root + "/"
	    + formatter.Format(m_playlist_template) + ".asx";

	pp = import::Playlist::Create(playlist_filename);
	pp->Load();

	TRACE << "Playlist is " << playlist_filename << "\n";

	album_filename = playlist_filename;
    }

    for (unsigned int i=0; i<m_ntracks; ++i)
    {
	db::RecordsetPtr rs = m_tracks[i].tags;
	util::Formatter formatter;

	std::string artist = rs->GetString(mediadb::ARTIST);
	formatter.AddField('a', util::ProtectLeafname(artist));
	formatter.AddField('A', util::ProtectLeafname(UnThe(artist)));
	formatter.AddField('c',
		  util::ProtectLeafname(rs->GetString(mediadb::ALBUM)));
	formatter.AddField('y',
		  util::ProtectLeafname(rs->GetString(mediadb::YEAR)));
	formatter.AddField('t',
		  util::ProtectLeafname(rs->GetString(mediadb::TITLE)));
	formatter.AddField('n',
		  util::ProtectLeafname(rs->GetString(mediadb::TRACKNUMBER)));

	std::string filename = "/" + formatter.Format(m_track_template);

	TRACE << "Renaming to " << filename << ".ext\n";

	std::string flacname = m_flac_root + filename + ".flac";
	std::string mp3name  = m_mp3_root  + filename + ".mp3";

	if (album_filename.empty())
	    album_filename = util::GetDirName(mp3name.c_str());

	/** Because they are disk-bound not CPU-bound, punt tagging operations
	 * to the disk queue.
	 */
	m_tracks[i].etp_flac->RenameAndTag(flacname, rs, m_disk_queue);
	m_tracks[i].etp_mp3 ->RenameAndTag(mp3name,  rs, m_disk_queue);

	if (pp)
	{
	    unsigned int track_number = rs->GetInteger(mediadb::TRACKNUMBER);
	    pp->SetEntry(track_number, mp3name);
	}
    }

    if (pp)
    {
	pp->Save();
    }

    std::string link = m_mp3_root + "/New Albums/" + 
	util::ProtectLeafname(album_title);
    util::MkdirParents(link.c_str());
    util::posix::MakeRelativeLink(link, m_mp3_root + "/" + album_filename);

    return 0;
}

} // namespace import
