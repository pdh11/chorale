#include "flac_encoder.h"
#include "config.h"
#include "libutil/trace.h"
#include "libutil/errors.h"
#include "libutil/stream.h"

#if HAVE_LIBFLAC
#include <FLAC/stream_encoder.h>
#include <FLAC/metadata.h>

namespace import {


        /* C callbacks */


namespace {

static FLAC__StreamEncoderWriteStatus FlacWrite(const FLAC__StreamEncoder*,
						const FLAC__byte *buffer,
						size_t bytes,
						unsigned, unsigned,
						void *handle)
{
    FlacEncoder *flac = (FlacEncoder*)handle;
    return (FLAC__StreamEncoderWriteStatus)flac->Write(buffer, bytes);
}

static FLAC__StreamEncoderSeekStatus FlacSeek(const FLAC__StreamEncoder*,
					      FLAC__uint64 pos,
					      void *handle)
{
    FlacEncoder *flac = (FlacEncoder*)handle;
    return (FLAC__StreamEncoderSeekStatus)flac->Seek(pos);
}

static FLAC__StreamEncoderTellStatus FlacTell(const FLAC__StreamEncoder*,
					      FLAC__uint64 *ppos,
					      void *handle)
{
    FlacEncoder *flac = (FlacEncoder*)handle;
    return (FLAC__StreamEncoderTellStatus)flac->Tell(ppos);
}

} // anonymous namespace


        /* C++ callbacks */


int FlacEncoder::Write(const unsigned char *buffer, size_t bytes)
{
    unsigned int rc = m_output->WriteAll(buffer, bytes);
    if (rc)
    {
	TRACE << "Flac output write all returned error " << rc << "\n";
	return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
    }

    return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
}

int FlacEncoder::Seek(uint64_t pos)
{
    m_output->Seek(pos);
    return FLAC__STREAM_ENCODER_SEEK_STATUS_OK;
}

int FlacEncoder::Tell(uint64_t *ppos)
{
    *ppos = m_output->Tell();
    return FLAC__STREAM_ENCODER_TELL_STATUS_OK;
}


        /* AudioEncoder methods */


unsigned int FlacEncoder::Init(util::Stream *output,
			       size_t sample_pairs)
{
    m_output = output;
    FLAC__StreamEncoder *enc = FLAC__stream_encoder_new();
    m_flac = enc;

    // flac -8
    FLAC__stream_encoder_set_max_lpc_order(enc, 12); // -l 12
    FLAC__stream_encoder_set_blocksize(enc, 4608);   // -b 4608
    FLAC__stream_encoder_set_do_mid_side_stereo(enc, true);
    FLAC__stream_encoder_set_loose_mid_side_stereo(enc, false); // -m
    FLAC__stream_encoder_set_do_exhaustive_model_search(enc, true); // -e
    FLAC__stream_encoder_set_max_residual_partition_order(enc, 6); // -r 6

    FLAC__StreamMetadata *md =
	FLAC__metadata_object_new(FLAC__METADATA_TYPE_SEEKTABLE);
    
    m_seektable = md;
    
    FLAC__stream_encoder_set_metadata(enc, &md, 1);

    FLAC__stream_encoder_set_total_samples_estimate(enc, sample_pairs);

    unsigned int nseekpoints = (unsigned)(sample_pairs/441000);
    if (nseekpoints < 64)
	nseekpoints = 64;

    FLAC__bool ok = FLAC__metadata_object_seektable_template_append_spaced_points(md, nseekpoints, sample_pairs);
    ok = ok && FLAC__metadata_object_seektable_template_sort(md, true);

    FLAC__StreamEncoderInitStatus state
	= FLAC__stream_encoder_init_stream(enc,
					   &FlacWrite,
					   &FlacSeek,
					   &FlacTell,
					   NULL,
					   this);

    if (!ok || state != FLAC__STREAM_ENCODER_INIT_STATUS_OK)
    {
	TRACE << "Flac encoder init failed: " << state << "\n";
	return EINVAL;
    }
    TRACE << "Flac encoder inited\n";
    return 0;
}

unsigned int FlacEncoder::Encode(const short *samples, size_t sample_pairs)
{
    if (!m_flac)
    {
	TRACE << "Flac encoder called with invalid encoder " << (void*)this
	      << "\n";
	return EINVAL;
    }

    FLAC__int32 *ibuf = new FLAC__int32[sample_pairs*2];

    for (unsigned int i=0; i<sample_pairs*2; ++i)
	ibuf[i] = samples[i];

    bool ok = FLAC__stream_encoder_process_interleaved(
	(FLAC__StreamEncoder*)m_flac,
	ibuf, 
	(unsigned)sample_pairs);

    delete[] ibuf;

    if (!ok)
    {
	FLAC__StreamEncoderState st = FLAC__stream_encoder_get_state(
	    (FLAC__StreamEncoder*)m_flac);
	TRACE << "Flac encoding failed (" << st << ")\n";
	return EINVAL;
    }

    return 0;
}

unsigned int FlacEncoder::Finish()
{
    FLAC__stream_encoder_finish((FLAC__StreamEncoder*)m_flac);

    FLAC__metadata_object_delete((FLAC__StreamMetadata*)m_seektable);
    m_seektable = NULL;
    FLAC__stream_encoder_delete((FLAC__StreamEncoder*)m_flac);
    m_flac = NULL;
    return 0;
}

} // namespace import

#endif // HAVE_LIBFLAC
