#include "encoding_task.h"
#include "config.h"
#include "tags.h"
#include "audio_encoder.h"
#include "tag_rename_task.h"
#include "libdb/recordset.h"
#include "libutil/bind.h"
#include "libutil/file.h"
#include "libutil/trace.h"
#include "libutil/errors.h"
#include "libutil/scheduler.h"
#include "libutil/file_stream.h"
#include "libutil/async_write_buffer.h"
#include <stdio.h>

namespace import {

EncodingTask::EncodingTask(const std::string& name,
			   util::TaskQueue *cpu_queue, 
			   util::TaskQueue *disk_queue,
			   const std::string& output_filename,
			   AudioEncoder* (*factory)())
    : util::Task(name),
      m_cpu_queue(cpu_queue),
      m_disk_queue(disk_queue),
      m_output_filename(output_filename), 
      m_encoder(factory()),
      m_rename_stage(EARLY),
      m_state(STARTING),
      m_input_size(0),
      m_input_done(0)
{}

EncodingTask::~EncodingTask()
{
    delete m_encoder;
    TRACE << "~EncodingTask\n";
}

void EncodingTask::SetInputStream(std::unique_ptr<util::Stream>& stm, size_t size)
{
    m_input_stream = std::move(stm);
    m_input_size = size; 
}

/** Rename and tag.
 * To get the threading right, there are three cases:
 *
 *  - Called before encoding starts: tag during original write, always write
 *    to new filename
 *  - Called during encoding: wait until encoding ends, then rename and tag
 *    (in this task; a second task might end up running before we've finished)
 *  - Called after encoding finishes: a new task must be started.
 *
 * When deciding what to do here we must mutex ourselves from the task doing
 * the actual encoding.
 */
void EncodingTask::RenameAndTag(const std::string& new_filename,
				db::RecordsetPtr tags)
{
    util::Mutex::Lock lock(m_rename_mutex);
    if (m_rename_stage == LATE)
    {
	m_disk_queue->PushTask(TagRenameTask::Create(m_output_filename,
						     new_filename, tags));
	return;
    }
    m_rename_filename = new_filename;
    m_rename_tags = tags;
}

unsigned int EncodingTask::OnReadable()
{
    /* We've been called from a low-latency poller thread, so punt to the
     * high-latency CPU thread pool.
     *
     * Front? Yes, we've started so we'll finish.
     */
//    TRACE << "EncodingTask(" << (void*)this << ") readable, pushing\n";
    m_cpu_queue->PushTaskFront(util::Bind(EncodingTaskPtr(this)).To<&EncodingTask::Run>());
    return 0;
}

unsigned int EncodingTask::Run()
{
//    TRACE << "In EncodingTask::Run(" << (void*)this << ") in state "
//	  << m_state << "\n";

    switch (m_state)
    {
    case STARTING:
    {
	unsigned int rc = util::OpenFileStream(m_output_filename.c_str(),
					       util::WRITE|util::SEQUENTIAL,
					       &m_output_stream);
	if (rc) {
            TRACE << "Can't open " << m_output_filename << "\n";
            exit(1);
	    return rc;
        }

	m_buffer_stream.reset(new util::AsyncWriteBuffer(m_output_stream.get(),
							 m_disk_queue));

	rc = m_encoder->Init(m_buffer_stream.get(), m_input_size/4);
	if (rc)
	    return rc;
	m_state = RUNNING;
	/* fall through */
    }
    case RUNNING:
    {
	enum { BUFSIZE = 1176*20 }; // In shorts

	if (!m_buffer)
	    m_buffer.reset(new short[BUFSIZE]);

	for (;;)
	{
	    size_t nbytes;
//	    TRACE << "Reading\n";
	    unsigned rc = m_input_stream->Read(m_buffer.get(),
					       BUFSIZE*sizeof(short),
					       &nbytes);

	    if (rc == EWOULDBLOCK)
	    {
		assert(m_input_stream->GetStreamFlags() & util::Stream::WAITABLE);
		assert(m_cpu_queue);
//		TRACE << "EncodingTask::Run(" << (void*)this << ") waits\n";
		return m_input_stream->Wait(
		    util::Bind(EncodingTaskPtr(this)).To<&EncodingTask::OnReadable>());
	    }
	
	    if (rc)
	    {
		TRACE << "Encode read failed: " << rc << "\n";
		return rc;
	    }

//	    TRACE << "Encode read got " << nbytes << " bytes\n";

	    if (!nbytes)
	    {
		TRACE << "Got EOF (" << m_input_done << "/" << m_input_size << "), finishing\n";
		break;
	    }

	    size_t nsamples = nbytes/4;

//	    TRACE << "Calling Encode\n";

	    rc = m_encoder->Encode(m_buffer.get(), nsamples);

//	    TRACE << "Encode returned " << rc << "\n";

	    if (rc)
	    {
		TRACE << "Encode failed: " << rc << "\n";
		return rc;
	    }
	    
	    m_input_done += nbytes;
	    
	    FireProgress((unsigned)m_input_done, (unsigned)m_input_size);

	    // and go round again
	}
	    
	FireProgress((unsigned)m_input_done, (unsigned)m_input_size);

//	TRACE << "Calling encoder::Finish\n";

	unsigned rc = m_encoder->Finish();
	if (rc)
	{
	    TRACE << "Encoder finish failed: " << rc << "\n";
	    return rc;
	}

//	TRACE << "Resetting streams\n";

	m_input_stream.reset(NULL);
	m_buffer_stream.reset(NULL);
	m_output_stream.reset(NULL);

#if !HAVE_LAME_GET_LAMETAG_FRAME

//	TRACE << "Calling encoder::PostFinish\n";

	/** @todo Once Debian stable gets Lame 3.98, get rid of this nastiness.
	 */
	m_encoder->PostFinish(m_output_filename);
#endif

	delete m_encoder;
	m_encoder = NULL;

//	TRACE << "Locking\n";

	{
	    util::Mutex::Lock lock(m_rename_mutex);
	    m_rename_stage = LATE;
	    if (!m_rename_filename.empty())
	    {
		util::RenameWithMkdir(m_output_filename.c_str(), 
				      m_rename_filename.c_str());
//		TRACE << "Tag point 2\n";
		import::TagWriter tags;
		tags.Init(m_rename_filename);
		tags.Write(m_rename_tags.get());
		m_rename_filename.clear();
	    }
	}

	TRACE << "et" << (void*)this << ": encoding finished\n";
	m_state = DONE;
    }
    case DONE:
    default:
	m_buffer.reset();
	break;
    }
    return 0;
}

} // namespace import

#ifdef TEST
# include "mp3_encoder.h"
# include "flac_encoder.h"
# include "libutil/worker_thread_pool.h"

class TestObserver: public util::TaskObserver
{
    bool m_done;
    unsigned int m_percent;
public:
    TestObserver() : m_done(false), m_percent(0) {}

    void OnProgress(const util::Task*, unsigned num, unsigned denom)
    {
	if (num == denom)
	    m_done = true;
	while (denom > 10000)
	{
	    num /= 2;
	    denom /= 2;
	}
	unsigned int percent = num*100/denom;
	if (percent != m_percent)
	{
	    m_percent = percent;
	    TRACE << percent << "%\n";
	}
    }

    bool IsDone() const { return m_done; }
};

int main()
{
#if HAVE_LAME
    std::unique_ptr<util::Stream> ifs;
    unsigned int rc = util::OpenFileStream("track.pcm", 
					   util::READ, &ifs);
    if (rc == ENOENT)
    {
	fprintf(stderr, "Can't find test track 'track.pcm', skipping test\n");
	return 0;
    }

    assert(!rc);
    assert(ifs.get());

    util::WorkerThreadPool wtp(util::WorkerThreadPool::LOW, 8);

    import::EncodingTaskPtr etp(new import::EncodingTask("test", NULL, &wtp,
							 "track.mp3",
							 &import::CreateMP3Encoder));
   
    assert(etp);
 
    TestObserver testobserver;
    etp->SetObserver(&testobserver);
    etp->SetInputStream(ifs, ifs->GetLength());

    while (!testobserver.IsDone())
    {
	assert(etp);

	rc = etp->Run();
	assert(rc == 0);
    }
#endif // HAVE_LAME

    return 0;
}

#endif // TEST
