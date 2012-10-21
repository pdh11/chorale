#include "encoding_task.h"
#include "tag_rename_task.h"
#include "tags.h"
#include "libmediadb/schema.h"
#include "vorbis_comment.h"
#include "libutil/trace.h"
#include "libutil/file.h"
#include <FLAC/stream_encoder.h>
#include <FLAC/metadata.h>
#include <sys/stat.h>
#include <sstream>
#include <lame/lame.h>

namespace import {

EncodingTaskPtr FlacEncodingTask::Create(const std::string& filename)
{
    return EncodingTaskPtr(new FlacEncodingTask(filename));
}

EncodingTaskPtr MP3LameEncodingTask::Create(const std::string& filename)
{
    return EncodingTaskPtr(new MP3LameEncodingTask(filename));
}

EncodingTask::~EncodingTask()
{
//    TRACE << "~EncodingTask\n";
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
				db::RecordsetPtr tags, util::TaskQueue *queue)
{
    boost::mutex::scoped_lock lock(m_rename_mutex);
    if (m_rename_stage == LATE)
    {
	queue->PushTask(TagRenameTask::Create(m_output_filename, new_filename,
					      tags));
	return;
    }
    m_rename_filename = new_filename;
    m_rename_tags = tags;
}

/** @todo Encode to a (vast) memory buffer and write in one go (less
 * fragmentation on >1 CPU boxes)
 */
void FlacEncodingTask::Run()
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

void MP3LameEncodingTask::Run()
{
    unsigned int totalsamples = m_input_size / 4;
    
    lame_t gfp = lame_init();

    int rc2 = lame_set_preset(gfp, STANDARD);
    if (rc2<0)
    {
	TRACE << "lame_set_preset returned " << rc2 << "\n";
	return;
    }
    rc2 = lame_init_params(gfp);
    if (rc2<0)
    {
	TRACE << "lame_init_params returned " << rc2 << "\n";
	return;
    }

    const int LUMP = 128*1024; // bytes

    short *sbuf = new short[LUMP/2];
    unsigned char *obuf = new unsigned char[LUMP];

    unsigned int samples_done = 0;
    
    FILE *f = fopen(m_output_filename.c_str(), "wb+");

    if (!f)
    {
	TRACE << "Lame encoder can't open '" << m_output_filename << "'\n";
	return;
    }

    for (;;) 
    {
	size_t nbytes;
	unsigned rc = m_input_stream->Read(sbuf, LUMP, &nbytes);
	if (rc)
	{
	    TRACE << "Lame encode read failed: " << rc << "\n";
	    break;
	}

	if (!nbytes)
	    break;

	size_t nsamples = nbytes/4;

	if (!nsamples)
	    break;
	
	rc2 = lame_encode_buffer_interleaved(gfp, sbuf, nsamples, obuf, LUMP);
	if (rc2 < 0)
	{
	    TRACE << "Lame encode failed: " << rc2 << "\n";
	    break;
	}

	int rc3 = fwrite(obuf, 1, rc2, f);
	if (rc3 != rc2)
	{
	    TRACE << "Lame encode write failed\n";
	}

	samples_done += nsamples;

	FireProgress(samples_done, totalsamples);
    }

    int rc = lame_encode_flush(gfp, obuf, LUMP);

    rc2 = fwrite(obuf, 1, rc, f);
	
    lame_mp3_tags_fid(gfp, f);
    lame_close(gfp);
    fclose(f);

    delete[] sbuf;
    delete[] obuf;

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

//    TRACE << "MP3 encoding finished\n";
}

}; // namespace import
