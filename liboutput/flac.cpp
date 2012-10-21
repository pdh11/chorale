#include "flac.h"
#include "libutil/trace.h"
#include <FLAC++/decoder.h>
#include <errno.h>

namespace output {

class FlacStream::Impl: public FLAC::Decoder::Stream
{
    util::SeekableStreamPtr m_stream;
    pos64 m_downstream_pos;
    void *m_buffer;
    size_t m_nbytes;

public:
    Impl(util::SeekableStreamPtr stream)
	: m_stream(stream),
	  m_downstream_pos(0),
	  m_buffer(NULL)
    {
	set_metadata_ignore_all();
	init();
    }

    // Being a FLAC::Decoder::Stream
    ::FLAC__StreamDecoderReadStatus read_callback(FLAC__byte buffer[],
						  size_t *bytes);
    ::FLAC__StreamDecoderSeekStatus seek_callback(FLAC__uint64 absolute_byte_offset);
    ::FLAC__StreamDecoderTellStatus tell_callback(FLAC__uint64 *absolute_byte_offset);
    ::FLAC__StreamDecoderLengthStatus length_callback(FLAC__uint64 *stream_length);
    bool eof_callback();
    ::FLAC__StreamDecoderWriteStatus write_callback(const ::FLAC__Frame *frame,
						    const FLAC__int32 *const buffer[]);
    void error_callback(::FLAC__StreamDecoderErrorStatus) {}

    // Being (like) a SeekableStream
    unsigned Read(void *buffer, size_t len, size_t *pread);
    void Seek(pos64 pos);
    pos64 Tell();
    pos64 GetLength();
};

::FLAC__StreamDecoderReadStatus 
FlacStream::Impl::read_callback(FLAC__byte buffer[], size_t *bytes)
{
    size_t inbytes = *bytes;
    unsigned rc = m_stream->Read(buffer, inbytes, bytes);
    if (rc)
	return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
    if (*bytes == 0)
	return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
    return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}

::FLAC__StreamDecoderSeekStatus FlacStream::Impl::seek_callback(FLAC__uint64 absolute_byte_offset)
{
    m_stream->Seek(absolute_byte_offset);
    return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

::FLAC__StreamDecoderTellStatus FlacStream::Impl::tell_callback(FLAC__uint64 *absolute_byte_offset)
{
    *absolute_byte_offset = m_stream->Tell();
    return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

::FLAC__StreamDecoderLengthStatus FlacStream::Impl::length_callback(FLAC__uint64 *stream_length)
{
    *stream_length = m_stream->GetLength();
    return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
}

bool FlacStream::Impl::eof_callback()
{
    return m_stream->Tell() == m_stream->GetLength();
}

::FLAC__StreamDecoderWriteStatus
FlacStream::Impl::write_callback(const ::FLAC__Frame *frame,
				 const FLAC__int32 *const buffer[])
{
    size_t samples = frame->header.blocksize;

    m_downstream_pos += samples*4;

    if (samples*4 > m_nbytes)
    {
	TRACE << "Warning, buffer too small, discarding some samples\n";
	samples = m_nbytes/4;
    }
    assert(m_buffer != NULL);

    const FLAC__int32 *ch0 = buffer[0];
    const FLAC__int32 *ch1 = buffer[1];
    uint32_t *output = (uint32_t*)m_buffer;

    for (unsigned int i=0; i<samples; ++i)
    {
	*output++ = ((*ch0++) & 0xFFFF) | ((*ch1++) << 16);
    }
    m_nbytes = samples*4;

    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

unsigned FlacStream::Impl::Read(void *buffer, size_t len, size_t *pread)
{
    m_buffer = buffer;
    m_nbytes = len;
    bool rc = process_single();
    m_buffer = NULL;
    if (!rc)
    {
	TRACE << "ps failed " << get_state() << "\n";
	*pread = 0;
    }
    else
	*pread = m_nbytes;
    return 0;
}

void FlacStream::Impl::Seek(pos64 pos)
{
    seek_absolute(pos/4);
    m_downstream_pos = pos;
}

util::SeekableStream::pos64 FlacStream::Impl::Tell()
{
    return m_downstream_pos;
}

util::SeekableStream::pos64 FlacStream::Impl::GetLength()
{
    return get_total_samples();
}


        /* FlacStream */


FlacStream::FlacStream(util::SeekableStreamPtr upstream)
    : m_impl(new Impl(upstream))
{
}

FlacStream::~FlacStream()
{
    delete m_impl;
}

unsigned FlacStream::Read(void *buffer, size_t len, size_t *pread)
{
    return m_impl->Read(buffer, len, pread);
}

unsigned FlacStream::Write(const void*, size_t, size_t*)
{
    return EINVAL;
}

void FlacStream::Seek(pos64 pos)
{
    m_impl->Seek(pos);
}

util::SeekableStream::pos64 FlacStream::Tell()
{
    return m_impl->Tell();
}

util::SeekableStream::pos64 FlacStream::GetLength()
{
    return m_impl->GetLength();
}

unsigned FlacStream::SetLength(pos64)
{
    return EINVAL;
}

}; // namespace output
