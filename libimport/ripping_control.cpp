#include "ripping_control.h"
#include "cd_drive.h"
#include "playlist.h"
#include "eject_task.h"
#include "mp3_encoder.h"
#include "flac_encoder.h"
#include "ripping_task.h"
#include "encoding_task.h"
#include "libdb/free_rs.h"
#include "libutil/file.h"
#include "libutil/trace.h"
#include "libutil/printf.h"
#include "libutil/formatter.h"
#include "libmediadb/schema.h"
#include <stdio.h>

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
        m_tracks[i].tags->SetInteger(mediadb::TRACKNUMBER, i+1);
    }

    for (unsigned int i=0; i<m_ntracks; ++i)
    {
	char leaf[32];
	sprintf(leaf, "cd%03ut%02u", this_index, i+1);
	std::string filename = m_flac_root + "/" + leaf;

	std::string task_name = util::Printf() << "flac-" << (i+1);

	import::EncodingTaskPtr etp1(new EncodingTask(task_name,
						      cpu_queue, disk_queue,
						      filename + ".flac",
						      &CreateFlacEncoder));
	m_tracks[i].etp_flac = etp1;
	etp1->SetObserver(this);

	filename = m_mp3_root + "/" + leaf;

	task_name = util::Printf() << "mp3-" << (i+1);

	import::EncodingTaskPtr etp2(new EncodingTask(task_name,
						      cpu_queue, disk_queue,
						      filename + ".mp3",
						      &CreateMP3Encoder));
	m_tracks[i].etp_mp3 = etp2;
	etp2->SetObserver(this);

	filename = std::string("/tmp/") + leaf;

	import::RippingTaskPtr rtp = 
	    import::RippingTask::Create(cd, i, filename, etp1, etp2,
					m_cpu_queue,
					m_disk_queue);
	m_tracks[i].rtp = rtp;
	rtp->SetObserver(this);
	drive->GetTaskQueue()->PushTask(
	    util::Bind(rtp).To<&RippingTask::Run>());
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
	if (m_tracks[i].etp_flac && task == m_tracks[i].etp_flac.get())
	    type = PROGRESS_FLAC;
	else if (m_tracks[i].etp_mp3 && task == m_tracks[i].etp_mp3.get())
	    type = PROGRESS_MP3;
	else if (m_tracks[i].rtp && task == m_tracks[i].rtp.get())
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

void RippingControl::SetAlbumName(const char *name)
{
    m_album_name = name;
}

unsigned int RippingControl::Done()
{
    // Cheat by using first track's details
    std::string album_artist = m_tracks[0].tags->GetString(mediadb::ARTIST);
    std::string album_year = m_tracks[0].tags->GetString(mediadb::YEAR);

    // Filename (playlist or dir) representing entire album
    std::string album_filename;

    std::string playlist_filename;
    std::list<std::string> playlist_entries;

    if (!m_playlist_template.empty())
    {
	util::Formatter formatter;

	formatter.AddField('a', util::ProtectLeafname(album_artist));
	formatter.AddField('A', util::ProtectLeafname(UnThe(album_artist)));
	formatter.AddField('c', util::ProtectLeafname(m_album_name));
	formatter.AddField('y', util::ProtectLeafname(album_year));
	
	playlist_filename = m_mp3_root + "/"
	    + formatter.Format(m_playlist_template) + ".asx";

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
	m_tracks[i].etp_flac->RenameAndTag(flacname, rs);
	m_tracks[i].etp_mp3 ->RenameAndTag(mp3name,  rs);

	playlist_entries.push_back(mp3name);
    }

    if (!playlist_filename.empty())
    {
	Playlist playlist;
	playlist.Init(playlist_filename);
	playlist.Save(&playlist_entries);
    }

    std::string link = m_mp3_root + "/New Albums/" + 
	util::ProtectLeafname(m_album_name);
    util::MkdirParents(link.c_str());
    util::posix::MakeRelativeLink(link, album_filename);

    return 0;
}

} // namespace import

#ifdef TEST
# include "test_cd.h"
# include "libutil/worker_thread_pool.h"
# include <string.h>

class TextObserver: public import::RippingControlObserver
{
    util::TaskQueue *m_cpu_queue;
    unsigned int m_percent[18]; // 6 tracks * 3 figures

public:
    explicit TextObserver(util::TaskQueue *cpu_queue);
    void OnProgress(unsigned int which, unsigned int type,
		    unsigned int num, unsigned int denom);

    bool IsDone();
};

TextObserver::TextObserver(util::TaskQueue *cpu_queue)
    : m_cpu_queue(cpu_queue),
      m_percent()
{
}

void TextObserver::OnProgress(unsigned int which, unsigned int type,
			      unsigned int num, unsigned int denom)
{
    //fprintf(stderr, "%u %u %u %u\n", which, type, num, denom);
    which = (which*3) + type - 1;
    unsigned int percent;
    if (num == denom)
	percent = 100;
    else
    {
	while (denom > 10000)
	{
	    denom /= 2;
	    num /= 2;
	}
	percent = num*100/denom;
	if (percent == 100)
	    percent = 99;
    }

    if (m_percent[which] != percent)
    {
	m_percent[which] = percent;
/*	TRACE << "[" << m_cpu_queue->Count() << "] "
	      << m_percent[0] << "% "
	      << m_percent[1] << "% "
	      << m_percent[2] << "% "
	      << m_percent[3] << "% "
	      << m_percent[4] << "% "
	      << m_percent[5] << "% "
	      << m_percent[6] << "% "
	      << m_percent[7] << "% "
	      << m_percent[8] << "% "
	      << m_percent[9] << "% "
	      << m_percent[10] << "% "
	      << m_percent[11] << "%\n";
*/
    }
}

int main()
{
    import::CDDrivePtr drive(new import::TestCDDrive);
    import::AudioCDPtr cd(new import::TestCD);
    
    util::WorkerThreadPool disk_pool(util::WorkerThreadPool::NORMAL, 30);
    util::WorkerThreadPool cpu_pool(util::WorkerThreadPool::LOW, 2);

    import::RippingControl rc(drive, cd, &cpu_pool, &disk_pool, ".", ".");
    TextObserver tobs(&cpu_pool);
    rc.AddObserver(&tobs);

    do {
	sleep(1);
    } while (disk_pool.Count() || cpu_pool.Count());

    TRACE << "Exiting\n";

    return 0;
}

#endif
