#include "config.h"
#include "ripping_task.h"
#include "libutil/file_stream.h"
#include "libutil/memory_stream.h"
#include "libutil/multi_stream.h"
#include "libutil/trace.h"
#include "libutil/async_write_buffer.h"
#include "audio_cd.h"

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
    int start = (m_cd->begin()+m_track)->first_sector;
    int end   = (m_cd->begin()+m_track)->last_sector;

//    TRACE << "Ripping track " << m_track+1 << " sectors "
//	  << start << ".." << end << "\n";

    unsigned int pcmsize = (unsigned)(end-start+1) * 2352;

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
	util::SeekableStreamPtr fsp;
	if (util::OpenFileStream(m_filename.c_str(), util::TEMP, &fsp) != 0)
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

    util::StreamPtr cdstream = m_cd->GetTrackStream(m_track);
    if (!cdstream)
    {
	TRACE << "Can't make CD stream\n";
	FireError(ENOENT);
	return;
    }

    char buf[2352*10];

    time_t t = time(NULL);

    size_t done = 0;
    while (done < pcmsize)
    {
	size_t lump = pcmsize - done;
	if (lump > sizeof(buf))
	    lump = sizeof(buf);

	size_t nread;
	rc = cdstream->Read(buf, lump, &nread);
	if (rc != 0)
	{
	    FireError(rc);
	    TRACE << "Read error " << rc << "\n";
	    return;
	}
	rc = ms->WriteAll(buf, nread);
	if (rc != 0)
	{
	    TRACE << "Write error " << rc << "\n";
	    FireError(rc);
	    return;
	}

//	TRACE << "Got " << nread << " bytes\n";

	done += nread;

	FireProgress((unsigned int)done, pcmsize);
    }
    
    t = time(NULL) - t;

    double x = ((end-start+1)/75.0)/(double)t;

    TRACE << "Rip track " << (m_track+1) << " done " << x << "x\n";
}

} // namespace import
