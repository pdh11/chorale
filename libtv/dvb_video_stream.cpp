#include "config.h"
#include "dvb_video_stream.h"
#include "libutil/trace.h"
#include "libutil/printf.h"
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>

#if HAVE_DVB

#include <sys/ioctl.h>
#include <poll.h>
#include <linux/dvb/frontend.h>
#include <linux/dvb/dmx.h>

namespace tv {

namespace dvb {


        /* VideoStream */


VideoStream::VideoStream()
    : m_audio_fd(-1),
      m_video_fd(-1),
      m_started(false),
      m_pes_packet_remaining(0),
      m_fill(0),
      m_esdata(NULL),
      m_eslen(0)
{
}

VideoStream::~VideoStream()
{
    if (m_audio_fd >= 0)
	::close(m_audio_fd);
    if (m_video_fd >= 0)
	::close(m_video_fd);
}

unsigned int VideoStream::Open(unsigned int adaptor, unsigned int device,
			       unsigned int audiopid, unsigned int videopid)
{
    std::string devnode = util::SPrintf("/dev/dvb/adapter%u/demux%u",
					adaptor, device);
    m_audio_fd = ::open(devnode.c_str(), O_RDONLY|O_NONBLOCK);
    m_video_fd = ::open(devnode.c_str(), O_RDONLY|O_NONBLOCK);

    if (m_audio_fd < 0 || m_video_fd < 0)
    {
	TRACE << "errno=" << errno << "\n";
	return errno;
    }

    /* Set a big buffer */
    int rc = ioctl(m_audio_fd, DMX_SET_BUFFER_SIZE, 188*100);
    if (rc < 0)
    {
	TRACE << "Can't set PES audio buffer size: " << errno << "\n";
    }
    rc = ioctl(m_video_fd, DMX_SET_BUFFER_SIZE, 188*100);
    if (rc < 0)
    {
	TRACE << "Can't set PES video buffer size: " << errno << "\n";
    }

//    TRACE << "Setting filter for audio pid " << audiopid << "\n";
    
    struct dmx_pes_filter_params pfp;
    memset(&pfp, 0, sizeof(pfp));
    pfp.pid = (uint16_t)audiopid;
    pfp.input = DMX_IN_FRONTEND;
    pfp.output = (dmx_output_t)3; //DMX_OUT_TAP;
    pfp.pes_type = DMX_PES_OTHER;
    pfp.flags = DMX_IMMEDIATE_START;

    rc = ::ioctl(m_audio_fd, DMX_SET_PES_FILTER, &pfp);
    if (rc < 0)
    {
	TRACE << "Can't set PES audio filter: " << errno << "\n";
	return errno;
    }

    if (videopid)
    {
	TRACE << "Setting filter for video pid " << videopid << "\n";
    
	pfp.pid = (uint16_t)videopid;
	pfp.input = DMX_IN_FRONTEND;
	pfp.output = (dmx_output_t)3; //DMX_OUT_TAP;
	pfp.pes_type = DMX_PES_OTHER;
	pfp.flags = DMX_IMMEDIATE_START;

	rc = ::ioctl(m_video_fd, DMX_SET_PES_FILTER, &pfp);
	if (rc < 0)
	{
	    TRACE << "Can't set PES video filter: " << errno << "\n";
	    return errno;
	}
    }

    return 0;
}

unsigned int VideoStream::Read(void *buffer, size_t len, size_t *pread)
{
    *pread = 0;

    len -= (len%188); // Whole frames only please

    while (!*pread)
    {
	ssize_t rc = ::read(m_audio_fd, buffer, len);
	if (rc < 0)
	{
	    if (errno == EOVERFLOW)
	    {
		TRACE << "Audio overflow\n";
		continue;
	    }
	    else if (errno != EWOULDBLOCK)
		return (unsigned)errno;
	}
	else
	{
	    *pread += rc;

	    len -= rc;
	    if (!len)
		return 0;
	    buffer = (void*)((char*)buffer + rc);
	}
    
	rc = ::read(m_video_fd, buffer, len);
	if (rc < 0)
	{
	    if (errno == EOVERFLOW)
	    {
		TRACE << "Video overflow\n";
		continue;
	    }
	    return (unsigned)errno; // Including EWOULDBLOCK this time
	}
	else
	{
	    *pread += rc;

	    len -= rc;
	    if (!len)
		return 0;
	    buffer = (void*)((char*)buffer + rc);
	}
    }
    
    return 0;

#if 0

    if (!m_eslen)
    {
        if (!m_started)
	{
	    // Look for PES packet signature

	    do {
		if (m_fill < 1024)
		{
		    int rc = ::poll(&pfd, 1, 10000);
		    if (rc <= 0)
		    {
			TRACE << "Poll failed in VS\n";
		    }

		    rc = ::read(m_fd, m_buffer + m_fill, BUFFER - m_fill);
		    if (rc < 0)
		    {
			TRACE << "Read error in VideoStream " << errno << "\n";
			return errno;
		    }

		    if (rc == 0)
		    {
			TRACE << "EOF in VideoStream\n";
			*pread = 0;
			return 0;
		    }

		    TRACE << "Read " << rc << " bytes in VideoStream\n";

		    m_fill += rc;
		}
		 
		const unsigned char *prefix = (const unsigned char*)
		    memchr(m_buffer, '\0', m_fill - 8);
		if (prefix)
		{
		    if (prefix[1] == 0 && prefix[2] == 1)// && prefix[3] == 0xE0)
		    {
			TRACE << "Found PES sync\n";
			m_fill -= (prefix - m_buffer);
			memmove(m_buffer, prefix, m_fill);
			m_started = true;
		    }
		    else
		    {
			// Fake sync
			m_fill -= (prefix + 1 - m_buffer);
			memmove(m_buffer, prefix + 1, m_fill);
		    }
		}
		else
		{
		    // No sync at all
		    m_fill = 0;
		}
	    } while (!m_started);
	}
	
	if (m_fill < BUFFER)
	{
	    int rc = ::poll(&pfd, 1, 10000);
	    if (rc <= 0)
	    {
		TRACE << "Poll failed in VS\n";
	    }

	    rc = ::read(m_fd, m_buffer + m_fill, BUFFER - m_fill);
	    if (rc < 0)
	    {
		TRACE << "Read error in VideoStream " << errno << "\n";
		return errno;
	    }
	    
	    if (rc == 0)
	    {
		TRACE << "EOF in VideoStream\n";
		*pread = 0;
		return 0;
	    }
	    
	    TRACE << "Read " << rc << " bytes in VideoStream\n";
	    
	    m_fill += rc;
	}

	m_esdata = m_buffer;
	m_eslen = m_fill;
    }

    if (len > m_eslen)
	len = m_eslen;

    TRACE << "Returning " << len << "\n";

    memcpy(buffer, m_esdata, len);
    m_esdata += len;
    m_eslen -= len;

    if (m_eslen == 0)
    {
	m_fill -= (m_esdata - m_buffer);
	memmove(m_buffer, m_esdata, m_fill);
    }

    *pread = len;
    return 0;
#endif
}

unsigned int VideoStream::Write(const void*, size_t, size_t*)
{
    return EPERM;
}

} // namespace dvb

} // namespace tv

#endif // HAVE_DVB
