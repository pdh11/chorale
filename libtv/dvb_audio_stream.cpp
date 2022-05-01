#include "config.h"
#include "dvb_audio_stream.h"
#include "libutil/trace.h"
#include "libutil/printf.h"
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>

#if HAVE_DVB

//LOG_DECL(TV);

#include <sys/ioctl.h>
#include <poll.h>
#include <linux/dvb/frontend.h>
#include <linux/dvb/dmx.h>

namespace tv {

namespace dvb {

AudioStream::AudioStream()
    : m_fd(-1),
      m_started(false),
      m_pes_packet_remaining(0),
      m_fill(0),
      m_esdata(NULL),
      m_eslen(0)
{
}

AudioStream::~AudioStream()
{
    TRACE << "~dvb::AudioStream\n";
    if (m_fd >= 0)
	::close(m_fd);
}

unsigned int AudioStream::Open(unsigned int adaptor, unsigned int device,
			       unsigned int audiopid)
{
    std::string devnode = util::SPrintf("/dev/dvb/adapter%u/demux%u",
					adaptor, device);
    m_fd = ::open(devnode.c_str(), O_RDWR|O_NONBLOCK);
    TRACE << "open(\"" << devnode << "\")=" << m_fd << "\n";
    if (m_fd < 0)
    {
	TRACE << "errno=" << errno << "\n";
	return errno;
    }

    /* Set a big buffer */
    int rc = ioctl(m_fd, DMX_SET_BUFFER_SIZE, 256*1024);
    if (rc < 0)
    {
	TRACE << "Can't set PES buffer size: " << errno << "\n";
	// Not a fatal error
    }

    TRACE << "Setting filter for audio pid " << audiopid << "\n";
    
#if 1
    struct dmx_pes_filter_params pfp;
    memset(&pfp, 0, sizeof(pfp));
    pfp.pid = (uint16_t)audiopid;
    pfp.input = DMX_IN_FRONTEND;
    pfp.output = DMX_OUT_TAP;
    pfp.pes_type = DMX_PES_OTHER;
    pfp.flags = DMX_IMMEDIATE_START;

    rc = ioctl(m_fd, DMX_SET_PES_FILTER, &pfp);
    if (rc < 0)
    {
	TRACE << "Can't set PES audio filter: " << errno << "\n";
	return errno;
    }
#else
    struct dmx_sct_filter_params sfp;
    memset(&sfp, '\0', sizeof(sfp));

    sfp.pid = 0x0130; // what's this?
    sfp.timeout = 0;
    sfp.flags = DMX_IMMEDIATE_START;

    rc = ::ioctl(m_fd, DMX_SET_FILTER, &sfp);
    if (rc < 0)
    {
	TRACE << "Can't set filter: " << errno << "\n";
	return (unsigned)errno;
    }
#endif

    return 0;
}

/** When selecting a single PID from the demux, we get PES out (not TS).
 *
 * For audio purposes, we must demux that and get ES (which is what an MP2
 * file is).
 */
unsigned int AudioStream::Read(void *buffer, size_t len, size_t *pread)
{
#if 0 /* Uncomment this to see what the raw stream is like */
    ssize_t res = ::read(m_fd, buffer, len);
    if (res < 0)
	return errno;
    *pread = res;
    return 0;
#endif

    if (m_eslen == 0)
    {
	do {
	    if (!m_started)
	    {
		// Look for PES packet signature

		do {
		    if (m_fill < BUFFER)
		    {
			ssize_t rc = ::read(m_fd, m_buffer + m_fill, 
					    BUFFER - m_fill);
			if (rc < 0)
			{
			    if (errno != EWOULDBLOCK)
				TRACE << "Read error in AudioStream "
				      << errno << "\n";
//			    else
//				LOG(TV) << "EWOULDBLOCK (starting)\n";

			    return errno;
			}
			
			if (rc == 0)
			{
			    TRACE << "EOF in AudioStream\n";
			    *pread = 0;
			    return 0;
			}

//			LOG(TV) << "Read " << rc << " bytes in AudioStream\n";

			m_fill += (unsigned)rc;
		    }
		    
		    const unsigned char *prefix = (const unsigned char*)
			memchr(m_buffer, '\0', m_fill - 4);
		    if (prefix)
		    {
			if (prefix[1] == 0 && prefix[2] == 1)// && prefix[3] == 0xC0)
			{
			    TRACE << "Found PES sync (" << prefix[3] << ")\n";
			    m_fill -= (unsigned)(prefix - m_buffer);
			    memmove(m_buffer, prefix, (unsigned int)m_fill);
			    m_started = true;
			}
			else
			{
			    // Fake sync
			    m_fill -= (unsigned)(prefix + 1 - m_buffer);
			    memmove(m_buffer, prefix + 1, (unsigned int)m_fill);
			}
		    }
		    else
		    {
			// No sync at all
			m_fill = 0;
		    }
		} while (!m_started);
	    }
	
	    // PES packet signature is at start of buffer
	    while (m_fill < 32)
	    {
		ssize_t rc = ::read(m_fd, m_buffer + m_fill, BUFFER - m_fill);
		if (rc < 0)
		{
		    if (errno != EWOULDBLOCK)
			TRACE << "Read error in AudioStream " << errno << "\n";
//		    else
//			LOG(TV) << "EWOULDBLOCK (running)\n";

		    if (errno != EOVERFLOW)
			return errno;
		}
		else if (rc == 0)
		{
		    TRACE << "EOF in AudioStream\n";
		    *pread = 0;
		    return 0;
		}
		else
		{
//		TRACE << "Read " << rc << " bytes in AudioStream\n";
		    m_fill += (unsigned)rc;
		}
	    }
	    
//	    TRACE << "m_fill now " << m_fill << "\n";
	    
	    if (m_pes_packet_remaining == 0)
	    {
		if (m_buffer[0] != 0 || m_buffer[1] != 0 || m_buffer[2] != 1)
		{
		    TRACE << "Lost PES sync\n" << util::Hex(m_buffer, 32);
		    m_started = false;
		}
	    }
	} while (!m_started);

	if (m_pes_packet_remaining == 0)
	{
	    // Start of packet
//	    unsigned int streamid = m_buffer[3];
	    unsigned int packlen = m_buffer[4]*256 + m_buffer[5] + 6;
	    unsigned int peshlen = m_buffer[8] + 9;
//	    TRACE << "id=" << streamid << " packlen=" << packlen << " peshlen=" << peshlen << "\n" << Hex(m_buffer, 16);
	    if (peshlen > 32) // Sanity check
	    {
		TRACE << "Clipping oversize header!\n";
		peshlen = 32;
	    }

	    m_esdata = m_buffer + peshlen;
	    m_eslen = m_fill - peshlen;
	    m_pes_packet_remaining = packlen - peshlen;
	    if (m_eslen > m_pes_packet_remaining)
		m_eslen = m_pes_packet_remaining;
	}
	else
	{
	    m_esdata = m_buffer;
	    m_eslen = m_fill;
	    if (m_eslen > m_pes_packet_remaining)
		m_eslen = m_pes_packet_remaining;
	}

	assert(m_pes_packet_remaining >= m_eslen);

	m_pes_packet_remaining -= m_eslen;
    }

    if (len > m_eslen)
	len = m_eslen;

//    LOG(TV) << "Returning " << len << "\n";

    memcpy(buffer, m_esdata, len);
    m_esdata += len;
    m_eslen -= (unsigned)len;

    if (m_eslen == 0)
    {
	m_fill -= (unsigned)(m_esdata - m_buffer);
//	TRACE << "Shuffling " << m_fill << "\n";
	memmove(m_buffer, m_esdata, m_fill);
    }

    *pread = len;
    return 0;

#if 0
    TRACE << "In read, eslen=" << m_eslen << " ppr=" << m_pes_packet_remaining
	  << "\n";
    if (!m_eslen)
    {
	unsigned int prefill = 0;

	do {
	    int rc = ::read(m_fd, m_buffer + prefill, TS_PACKET - prefill);
	    if (rc < 0)
	    {
		TRACE << "Read error in AudioStream " << errno << "\n";
		return errno;
	    }

	    if (rc == 0)
	    {
		TRACE << "EOF in AudioStream\n";
		*pread = 0;
		return 0;
	    }

	    TRACE << "Read " << rc << " bytes in AudioStream\n";
	
	    if (m_buffer[0] != 0x47)
	    {
		TRACE << "Bogus start-of-packet\n" << Hex(m_buffer, TS_PACKET);
		unsigned char *byte47 = (unsigned char*)memchr(m_buffer, 0x47,
							       TS_PACKET);
		if (byte47)
		{
		    prefill = TS_PACKET - (byte47 - m_buffer);
		    TRACE << "0x47 found at " << (byte47-m_buffer)
			  << " prefill is " << prefill << "\n";
		    memmove(m_buffer, byte47, prefill);
		}
		continue;
	    }

	    if (!m_started && ((m_buffer[1] & 0x40) != 0))
		m_started = true;

	    prefill = 0;

	} while (!m_started);
	
	bool adaptation = (m_buffer[3] & 0x20) != 0;

	const unsigned char *pesdata;
	unsigned int peslen;

	if (adaptation)
	{
	    unsigned int alen = m_buffer[4];
	    pesdata = m_buffer + 5 + alen;
	    peslen = TS_PACKET - alen - 5;
	}
	else
	{
	    pesdata = m_buffer + 5;
	    peslen = TS_PACKET - 4;
	}

	if (m_pes_packet_remaining == 0)
	{
	    // Start of packet
	    unsigned int packlen = pesdata[4]*256 + pesdata[5] + 6;
	    unsigned int peshlen = pesdata[8];
	    TRACE << "packlen=" << packlen << " peshlen=" << peshlen << "\n";
	    m_esdata = pesdata + peshlen + 9;
	    m_eslen = peslen - peshlen - 9;
	    m_pes_packet_remaining = packlen - peshlen - 9;
	}
	else
	{
	    m_esdata = pesdata;
	    m_eslen = peslen;
	    if (m_eslen > m_pes_packet_remaining)
		m_eslen = m_pes_packet_remaining;
	}

	m_pes_packet_remaining -= m_eslen;
    }

    if (len > m_eslen)
	len = m_eslen;

    memcpy(buffer, m_esdata, len);
    m_esdata += len;
    m_eslen -= len;
    *pread = len;
    return 0;
#endif
}

unsigned int AudioStream::Write(const void*, size_t, size_t*)
{
    return EPERM;
}

int AudioStream::GetHandle()
{ 
//    TRACE << "AudioStream: fd=" << m_fd << "\n";
    return m_fd;
}

} // namespace dvb

} // namespace tv

#endif // HAVE_DVB
