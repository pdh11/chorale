#include "config.h"
#include "ripping_task.h"

#ifdef HAVE_LIBCDIOP
#include "libutil/file_stream.h"
#include "libutil/memory_stream.h"
#include "libutil/multi_stream.h"
#include "libutil/trace.h"
#include "libutil/async_write_buffer.h"
#include "cd_drives.h"
#include <cdio/cdio.h>
#include <cdio/cd_types.h>
#include <cdio/paranoia.h>

namespace import {

RippingTaskPtr RippingTask::Create(AudioCDPtr cd, unsigned int track,
				   const std::string& filename,
				   EncodingTaskPtr etp1, EncodingTaskPtr etp2, 
				   util::TaskQueue *encode_queue, 
				   util::TaskQueue *disk_queue)
{
    return RippingTaskPtr(new RippingTask(cd, track, filename, etp1, etp2, 
					  encode_queue, disk_queue));
}

RippingTask::RippingTask(AudioCDPtr cd, unsigned int track,
			 const std::string& filename,
			 EncodingTaskPtr etp1, EncodingTaskPtr etp2,
			 util::TaskQueue *encode_queue, 
			 util::TaskQueue *disk_queue)
    : m_cd(cd), 
      m_track(track),
      m_filename(filename),
      m_etp1(etp1),
      m_etp2(etp2),
      m_encode_queue(encode_queue),
      m_disk_queue(disk_queue)
{
}

RippingTask::~RippingTask()
{
//    TRACE << "~RippingTask\n";
}

void RippingTask::Run()
{
    cdrom_drive_t *cdt = (cdrom_drive_t*)m_cd->GetHandle();
    
    lsn_t start = (m_cd->begin()+m_track)->firstsector;
    lsn_t end   = (m_cd->begin()+m_track)->lastsector;

//    TRACE << "Ripping track " << m_track+1 << " sectors "
//	  << start << ".." << end << "\n";

    size_t pcmsize = (end-start+1) * CDIO_CD_FRAMESIZE_RAW;

    // If there's CPU to spare, back to RAM, else back to disk
    util::SeekableStreamPtr backingstream;
    if (m_encode_queue->AnyWaiting())
    {
	util::MemoryStreamPtr msp;
	util::MemoryStream::Create(&msp, pcmsize);
	backingstream = msp;
//	TRACE << "Chosen to rip via RAM\n";
    }
    else
    {
	util::FileStreamPtr fsp;
	if (util::FileStream::CreateTemporary(m_filename.c_str(), &fsp) != 0)
	{
	    TRACE << "Can't create temporary\n";
	    return;
	}

//	StreamPtr awb;
//	AsyncWriteBuffer::Create(fsp, m_disk_queue, &awb);
	backingstream = fsp;
//	TRACE << "Chosen to rip via disk\n";
    }
    util::MultiStreamPtr ms;
    unsigned int rc = util::MultiStream::Create(backingstream, pcmsize, &ms);
    if (rc != 0)
    {
	FireError(rc);
	return;
    }

    util::StreamPtr pcmforflac;
    ms->CreateOutput(&pcmforflac);

    util::StreamPtr pcmformp3;
    ms->CreateOutput(&pcmformp3);

    m_etp1->SetInputStream(pcmforflac, pcmsize);
    m_encode_queue->PushTask(m_etp1);

    m_etp2->SetInputStream(pcmformp3, pcmsize);
    m_encode_queue->PushTask(m_etp2);

    cdrom_paranoia_t *p = paranoia_init(cdt);

    paranoia_modeset(p, PARANOIA_MODE_FULL^PARANOIA_MODE_NEVERSKIP);
    paranoia_seek(p, start, SEEK_SET);

    time_t t = time(NULL);

    for (lsn_t i = start; i<= end; ++i)
    {
	int16_t *buf = paranoia_read(p, NULL);
	rc = ms->WriteAll(buf, CDIO_CD_FRAMESIZE_RAW);
	if (rc != 0)
	{
	    FireError(rc);
	    return;
	}

	FireProgress(i-start+1, end-start+1);
    }
    
    t = time(NULL) - t;

    paranoia_free(p);

    double x = ((end-start+1)/75.0)/t;

    TRACE << "Rip track " << (m_track+1) << " done " << x << "x\n";
}

} // namespace import

#endif // HAVE_LIBCDIOP
