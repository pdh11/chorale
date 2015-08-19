#include "program_stream.h"
#include "libutil/errors.h"
#include "libutil/trace.h"
#include <algorithm>
#include <string.h>

namespace tv {

static const unsigned char mpegps_header[] =
{
    0x00, 0x00, 0x01, 0xBA,
    0x44, 0x00, 0x04, 0x00, 0x04, 0x01,
    0x00, 0x9C, 0x43,  // Mux rate, bytes/s/50 http://dvd.sourceforge.net/dvdinfo/packhdr.html
    0x00 
};

ProgramStream::ProgramStream(std::unique_ptr<util::Stream>& stream)
    : m_stream(std::move(stream)),
      m_output_buffer(mpegps_header, mpegps_header + sizeof(mpegps_header)),
      m_video_pid(0),
      m_audio_pid(0)
{
}

ProgramStream::~ProgramStream()
{
}

unsigned ProgramStream::Read(void *buffer, size_t len, size_t *pread)
{
    while (m_output_buffer.empty())
    {
	unsigned int rc = FillOutputBuffer();
	if (rc)
	    return rc;
    }

    size_t lump = std::min(len, m_output_buffer.size());
    memcpy(buffer, m_output_buffer.data(), lump);
    *pread = lump;
    m_output_buffer.erase(0,lump);
    return 0;
}

/** Read input until we've got a whole video or audio PES packet.
 *
 * Once we've got an entire packet (which we can tell, even in the
 * case of video, because we get the start-of-packet for the *next*
 * packet), we can "return" it by swapping it into m_output_buffer.
 *
 * It'd be nice to reduce memory use by only buffering audio packets,
 * sending video ones straight through -- but we can't, as we need to
 * rewrite the length fields at the beginnings of the video packets,
 * but we only know the length at the end.
 */
unsigned ProgramStream::FillOutputBuffer()
{
    unsigned char buffer[188];
    size_t nread;
    unsigned int rc = m_stream->Read(buffer, 188, &nread);
    if (rc)
	return rc;
    if (nread != 188)
	return EIO;

    if (buffer[0] != 0x47)
    {
	// This packet is bad, let's try and get another one
	return 0;
    }

    unsigned int pid = (buffer[1] & 0x1F) * 256 + buffer[2];

    bool adaptation = (buffer[3] & 0x20) != 0;

    const unsigned char *pesdata;
    unsigned int peslen;

    if (adaptation)
    {
	unsigned int alen = buffer[4];
	pesdata = buffer + 5 + alen;
	peslen = 188 - alen - 5;
    }
    else
    {
	pesdata = buffer + 4;
	peslen = 188 - 4;
    }

    if (buffer[1] & 0x40)
    {
	unsigned int stream = pesdata[3];
	if (stream >= 0xC0 && stream <= 0xDF && !m_audio_pid)
	{
	    m_audio_pid = pid;
//	    TRACE << "Found audio stream " << stream << " pid " << pid << "\n";
	}
	else if (stream >= 0xE0 && stream <= 0xEF && !m_video_pid)
	{
	    m_video_pid = pid;
//	    TRACE << "Found video stream " << stream << " pid " << pid << "\n";
	}

	if (pid == m_audio_pid && !m_audio_buffer.empty())
	{
//	    TRACE << "Sending " << m_audio_buffer.length() << "-byte audio packet\n";
	    std::swap(m_output_buffer, m_audio_buffer);
	}
	else if (pid == m_video_pid && !m_video_buffer.empty())
	{
	    size_t packet_length = m_video_buffer.length() - 6;
	    if (m_video_buffer[4] == 0 && m_video_buffer[5] == 0)
	    {
		m_video_buffer[4] = (unsigned char)(packet_length >> 8);
		m_video_buffer[5] = (unsigned char)packet_length;
	    }
//	    TRACE << "Sending " << packet_length << "-byte video packet\n";
	    std::swap(m_output_buffer, m_video_buffer);
	}
    }

    if (pid == m_audio_pid && m_audio_buffer.length() < 65535)
	m_audio_buffer.append(pesdata, pesdata+peslen);
    else if (pid == m_video_pid && m_video_buffer.length() < 65535)
	m_video_buffer.append(pesdata, pesdata+peslen);

    return 0;
}

unsigned ProgramStream::Write(const void*, size_t, size_t*)
{
    return EPERM;
}

int ProgramStream::GetHandle()
{ 
    return m_stream->GetHandle();
}

} // namespace tv
