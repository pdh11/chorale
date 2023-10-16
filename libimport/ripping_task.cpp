#include "config.h"
#include "ripping_task.h"
#include "libutil/file_stream.h"
#include "libutil/memory_stream.h"
#include "libutil/multi_stream.h"
#include "libutil/printf.h"
#include "libutil/trace.h"
#include "libutil/bind.h"
#include <time.h>
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
			 util::TaskQueue*)
    : Task(util::Printf() << "rip-" << (track+1)),
      m_cd(cd), 
      m_track(track),
      m_filename(filename),
      m_etp1(etp1),
      m_etp2(etp2),
      m_encode_queue(encode_queue)
{
}

RippingTask::~RippingTask()
{
    TRACE << "~RippingTask\n";
}

class AlignedBuffer
{
    void *m_buffer;

public:
    AlignedBuffer() : m_buffer(NULL) {}
    ~AlignedBuffer()
    {
        free(m_buffer);
    }

    int allocate(unsigned size)
    {
        return posix_memalign(&m_buffer, 4096, size);
    }

    void *get() { return m_buffer; }
};

unsigned int RippingTask::Run()
{
    int start = (m_cd->begin()+m_track)->first_sector;
    int end   = (m_cd->begin()+m_track)->last_sector;

//    TRACE << "Ripping track " << m_track+1 << " sectors "
//	  << start << ".." << end << "\n";

    unsigned int pcmsize = (unsigned)(end-start+1) * 2352;
    unsigned int rc;

    // If there's CPU to spare, back to RAM, else back to disk
    //
    // Backing to disk uses >25% of this thread's CPU; to RAM, <10% (which
    // still feels like a lot), mostly in faulting-in the pages.
    std::unique_ptr<util::Stream> backingstream;
    if (m_encode_queue->AnyWaiting())
    {
	backingstream.reset(new util::MemoryStream(pcmsize));
	TRACE << "Chosen to rip via RAM\n";
    }
    else
    {
	rc = util::OpenFileStream(m_filename.c_str(), util::TEMP, &backingstream);
	if (rc != 0)
	{
	    TRACE << "Can't create temporary\n";
	    return rc;
	}

//	StreamPtr awb;
//	AsyncWriteBuffer::Create(fsp, m_disk_queue, &awb);
	TRACE << "Chosen to rip via disk\n";
    }

    util::MultiStreamPtr msp = util::MultiStream::Create(backingstream,
							 pcmsize);
    // Note that msp now owns the backingstream

    std::unique_ptr<util::Stream> pcmforflac;
    msp->CreateOutput(&pcmforflac);

    std::unique_ptr<util::Stream> pcmformp3;
    msp->CreateOutput(&pcmformp3);

    m_etp1->SetInputStream(pcmforflac, pcmsize);
    m_encode_queue->PushTask(util::Bind(m_etp1).To<&EncodingTask::Run>());

    m_etp2->SetInputStream(pcmformp3, pcmsize);
    m_encode_queue->PushTask(util::Bind(m_etp2).To<&EncodingTask::Run>());

    std::unique_ptr<util::Stream> cdstream = m_cd->GetTrackStream(m_track);
    if (!cdstream.get())
    {
	TRACE << "Can't make CD stream\n";
	FireError(ENOENT);
	return ENOENT;
    }

    const unsigned SIZE = 2352*20;
    AlignedBuffer buf;
    rc = buf.allocate(SIZE);
    if (rc) {
        return rc;
    }

    time_t t = time(NULL);

    size_t done = 0;
    while (done < pcmsize)
    {
	size_t lump = pcmsize - done;
	if (lump > SIZE)
	    lump = SIZE;

	size_t nread;
	rc = cdstream->Read(buf.get(), lump, &nread);
	if (rc != 0 || nread == 0)
	{
	    FireError(rc);
	    TRACE << "Read error " << rc << "\n";
	    return rc;
	}
	rc = msp->WriteAll(buf.get(), nread);
	if (rc != 0)
	{
	    TRACE << "Write error " << rc << "\n";
	    FireError(rc);
	    return rc;
	}

//	TRACE << "Got " << nread << " bytes\n";

	done += nread;

	FireProgress((unsigned int)done, pcmsize);
    }

    t = time(NULL) - t;

    if (t)
    {
	double x = ((end-start+1)/75.0)/(double)t;

	TRACE << "Rip track " << (m_track+1) << " done " << x << "x\n";
    }
    return 0;
}

} // namespace import
