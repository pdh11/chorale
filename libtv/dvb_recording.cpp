#include "config.h"
#include "dvb_recording.h"
#include "dvb_service.h"
#include "libutil/trace.h"
#include "libutil/file_stream.h"
#include "libutil/async_write_buffer.h"
#include "libutil/worker_thread_pool.h"
#include "libutil/errors.h"
#include "libutil/scheduler.h"
#include "libutil/bind.h"
#include <unistd.h>
#include <fcntl.h>

namespace tv {

namespace dvb {

Recording::Recording(util::Scheduler *poller,
		     util::TaskQueue *disk_queue,
		     Service *service, unsigned int channel,
		     time_t start, time_t end,
		     const std::string& filename)
    : tv::Recording(poller, start, end),
      m_scheduler(poller),
      m_disk_queue(disk_queue),
      m_service(service),
      m_channel(channel),
      m_filename(filename),
      m_started(false)
{
}

unsigned int Recording::Run()
{
#if HAVE_DVB
    if (!m_started)
    {
	m_started = true;
	
	TRACE << "Recording starts (" << m_filename << ")\n";

	unsigned int rc = m_service->GetStream(m_channel, &m_input_stream);
	if (rc)
	{
	    TRACE << "Can't open stream\n";
	    return rc;
	}
	
	rc = util::OpenFileStream(m_filename.c_str(), 
				  util::WRITE | util::SEQUENTIAL,
				  &m_output_stream);
	if (rc != 0)
	{
	    TRACE << "Can't create output file: " << rc << "\n";
	    return rc;
	}

	m_buffer_stream.reset(
	    new util::AsyncWriteBuffer(m_output_stream.get(), m_disk_queue));
    }

    unsigned char buffer[1024];
	
    do {
	size_t n;
	unsigned int rc = m_input_stream->Read(buffer, sizeof(buffer), &n);

	if (rc == EWOULDBLOCK)
	{
	    m_scheduler->WaitForReadable(
		util::Bind(RecordingPtr(this)).To<&Recording::Run>(),
		m_input_stream->GetHandle());
	    return 0;
	}

	if (rc != 0)
	{
	    TRACE << "Read failed: " << rc << "\n";
	    return rc;
	}

	if (n == 0)
	{
	    TRACE << "EOF on stream\n";
	    return 0;
	}

	rc = m_buffer_stream->WriteAll(buffer, n);
	if (rc != 0)
	{
	    TRACE << "Write failed: " << rc << "\n";
	    return rc;
	}
    } while (time(NULL) < m_end);

    TRACE << "Recording ends\n";
#endif
    return 0;
}

} // namespace dvb

} // namespace tv

#ifdef TEST

int main()
{
    tv::dvb::Channels c;
    c.Load("/etc/channels.conf");

    unsigned int audio = 9999, video = 9999;

    for (tv::dvb::Channels::const_iterator i = c.begin(); i != c.end(); ++i)
    {
	if (i->videopid == 0 && i->audiopid != 0)
	{
	    audio = (int)(i - c.begin());
	    break;
	}
    }

    for (tv::dvb::Channels::const_iterator i = c.begin(); i != c.end(); ++i)
    {
	if (i->videopid != 0 && i->audiopid != 0)
	{
	    video = (int)(i - c.begin());
	    break;
	}
    }
    
    if (audio == 9999 || video == 9999)
    {
	TRACE << "Can't find suitable audio/video channels\n";
	return 0;
    }

    tv::dvb::Frontend f;
    unsigned int res = f.Open(0,0);
    if (res)
    {
	TRACE << "Can't open DVB frontend (no hardware?)\n";
	return 0;
    }

    time_t now = time(NULL);

    util::BackgroundScheduler poller;
    util::WorkerThreadPool wtp(util::WorkerThreadPool::HIGH);

    tv::dvb::Service service(&f, &c, "", &poller, &wtp);

    new tv::dvb::Recording(&poller, &wtp, &service,
			   audio,
			   now + 3, now + 8,
			   "grab.mp2");

    new tv::dvb::Recording(&poller, &wtp, &service,
			   audio,
			   now + 5, now + 15,
			   "grab2.mp2");

    new tv::dvb::Recording(&poller, &wtp, &service,
			   video,
			   now + 13, now + 18,
			   "grab.mpg");

    while ((time(NULL) - now) < 20)
    {
	poller.Poll(1000);
    }

    TRACE << "Exiting\n";
    return 0;
}

#endif
