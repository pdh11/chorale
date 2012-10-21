#include "config.h"
#include "encoding_task_flac.h"
#include "vorbis_comment.h"
#include "tags.h"
#include "libmediadb/schema.h"
#include "libutil/trace.h"
#include "libutil/file.h"
#include <sstream>

#ifdef HAVE_LIBFLAC
#include <FLAC/stream_encoder.h>
#include <FLAC/metadata.h>

namespace import {

EncodingTaskPtr EncodingTaskFlac::Create(const std::string& filename)
{
    return EncodingTaskPtr(new EncodingTaskFlac(filename));
}

/** @todo Encode to a (vast) memory buffer and write in one go (less
 * fragmentation on >1 CPU boxes)
 */
void EncodingTaskFlac::Run()
{
    FLAC__StreamEncoder *enc = FLAC__stream_encoder_new();

//    TRACE << "enc " << (void*)enc << "\n";

    // flac -8
    FLAC__stream_encoder_set_max_lpc_order(enc, 12); // -l 12
    FLAC__stream_encoder_set_blocksize(enc, 4608);   // -b 4608
    FLAC__stream_encoder_set_do_mid_side_stereo(enc, true);
    FLAC__stream_encoder_set_loose_mid_side_stereo(enc, false); // -m
    FLAC__stream_encoder_set_do_exhaustive_model_search(enc, true); // -e
    FLAC__stream_encoder_set_max_residual_partition_order(enc, 6); // -r 6

    FLAC__StreamMetadata *md[2];
    md[0] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_SEEKTABLE);
    md[1] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT);
    
    FLAC__stream_encoder_set_metadata(enc, md, 2);

    unsigned int totalsamples = m_input_size / 4;

    FLAC__stream_encoder_set_total_samples_estimate(enc, totalsamples);

    unsigned int nseekpoints = totalsamples/441000;
    if (nseekpoints < 64)
	nseekpoints = 64;

    FLAC__bool ok = FLAC__metadata_object_seektable_template_append_spaced_points(md[0], nseekpoints, totalsamples);
    ok = FLAC__metadata_object_seektable_template_sort(md[0], true);

    {
	boost::mutex::scoped_lock lock(m_rename_mutex);
	if (!m_rename_filename.empty())
	{
	    m_output_filename = m_rename_filename;
	    m_rename_filename.clear();
//	    TRACE << "Tag point 1\n";
	    
	    std::map<std::string, std::string> tags;
	    for (unsigned int i=0; i<mediadb::FIELD_COUNT; ++i)
	    {
		const char *s = import::GetVorbisTagForField(i);
		if (s)
		{
		    std::string val = m_rename_tags->GetString(i);
		    if (!val.empty())
			tags[s] = val;
		}
	    }

	    FLAC__metadata_object_vorbiscomment_resize_comments(md[1],
								tags.size());

	    typedef std::map<std::string, std::string> tags_t;
	    unsigned int j=0;
	    for (tags_t::const_iterator i = tags.begin();
		 i != tags.end();
		 ++i, ++j)
	    {
		std::ostringstream comment;
		comment << i->first << "=" << i->second;
		std::string cstr = comment.str();
		FLAC__StreamMetadata_VorbisComment_Entry entry;
		entry.entry = (unsigned char*)cstr.c_str();
		entry.length = cstr.length();
		FLAC__metadata_object_vorbiscomment_set_comment(md[1], j, 
								entry, true);
	    }
	}
    }

    util::MkdirParents(m_output_filename.c_str());

//    FLAC__stream_encoder_set_filename(enc, m_output_filename.c_str());

    /* Init -- all the set_x's must be above here */

    FLAC__StreamEncoderInitStatus state
	= FLAC__stream_encoder_init_file(enc,
					 m_output_filename.c_str(),
					 NULL, NULL);
    
    if (state != 0)
    {
	TRACE << "Encode failed on '" << m_output_filename << "'\n";
	return;
    }

    const int LUMP = 128*1024; // bytes

    short *sbuf = new short[LUMP/2];
    FLAC__int32 *ibuf = new FLAC__int32[LUMP/2];

    unsigned int samples_done = 0;

    for (;;) 
    {
	size_t nbytes;
	unsigned rc = m_input_stream->Read(sbuf, LUMP, &nbytes);
	if (rc)
	{
	    TRACE << "FLAC encode read failed: " << rc << "\n";
	    break;
	}

	if (!nbytes)
	    break;

	size_t nsamples = nbytes/4;

	if (!nsamples)
	    break;

	for (unsigned int i=0; i<nsamples*2; ++i)
	    ibuf[i] = sbuf[i];

	ok = FLAC__stream_encoder_process_interleaved(enc, ibuf, nsamples);

	samples_done += nsamples;

	FireProgress(samples_done, totalsamples);

	if (!ok)
	{
	    TRACE << "FLAC encoding failed\n";
	    break;
	}
    }

    delete[] sbuf;
    delete[] ibuf;

    FLAC__stream_encoder_finish(enc);

    FLAC__metadata_object_delete(md[0]);
    FLAC__metadata_object_delete(md[1]);
    FLAC__stream_encoder_delete(enc);

    {
	boost::mutex::scoped_lock lock(m_rename_mutex);
	m_rename_stage = LATE;
	if (!m_rename_filename.empty())
	{
	    util::RenameWithMkdir(m_output_filename.c_str(), 
				  m_rename_filename.c_str());
//	    TRACE << "Tag point 2\n";
	    import::WriteTags(m_rename_filename, m_rename_tags);
	    m_rename_filename.clear();
	}
    }

//    TRACE << "FLAC encoding finished\n";
}

}; // namespace import

#endif // HAVE_FLAC
