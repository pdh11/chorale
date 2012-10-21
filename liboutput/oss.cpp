#include "oss.h"
#include "libutil/trace.h"
#include <fcntl.h>
#include <linux/soundcard.h>
#include <sys/ioctl.h>
#include <errno.h>

namespace output {

OSSOutput::OSSOutput(util::PollerInterface *poller, Decoders *decoders)
    : m_fd(-1), m_bufstart(0), m_bufend(0), m_poller(poller),
      m_decoders(decoders)
{
}

void OSSOutput::OpenOutput()
{
    m_fd = open("/dev/dsp", O_RDWR|O_NONBLOCK);

    if (m_fd < 0)
	TRACE << "Can't open /dev/dsp: " << errno << "\n";

    int format = AFMT_S16_LE;
    ioctl(m_fd, SNDCTL_DSP_SETFMT, &format);
    int stereo = 1;
    ioctl(m_fd, SNDCTL_DSP_STEREO, &stereo);
    unsigned rate = 44100;
    ioctl(m_fd, SNDCTL_DSP_SPEED, &rate);

    m_poller->AddHandle(m_fd, this, util::PollerInterface::OUT);
    TRACE << "OSS Opened\n";
}

void OSSOutput::CloseOutput()
{
    m_poller->RemoveHandle(m_fd);
    close(m_fd);
    m_fd = -1;
}

unsigned OSSOutput::OnActivity()
{
    TRACE << "OA\n";
    for (;;)
    {
	if (m_bufstart != m_bufend)
	{
	    int rc = write(m_fd, m_buffer+m_bufstart, m_bufend-m_bufstart);
	    if (rc <= 0)
		return 0;
	    TRACE << "Wrote " << rc << "/" << m_bufend-m_bufstart << "\n";
	    m_bufstart += rc;
	}
	else
	{
	    if (!m_stream)
	    {
		m_stream = m_decoders->NextPCMStream();
		if (!m_stream)
		{
		    TRACE << "No stream, closing\n";
		    CloseOutput();
		    return 0;
		}
	    }

	    size_t nread = 0;
	    unsigned int rc = m_stream->Read(m_buffer, sizeof(m_buffer),
					     &nread);
	    if (nread == 0)
	    {
		m_stream = m_decoders->NextPCMStream();
		if (!m_stream)
		{
		    TRACE << "No stream, closing\n";
		    CloseOutput();
		    return 0;
		}
	    }
	    else
	    {
		m_bufstart = 0;
		m_bufend = nread;
	    }
	}
    }
}

}; // namespace output
