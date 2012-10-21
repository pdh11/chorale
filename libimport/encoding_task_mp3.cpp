#include "config.h"
#include "encoding_task_mp3.h"
#include "libdb/recordset.h"
#include "libutil/trace.h"
#include "libutil/file.h"
#include "tags.h"
#include "libutil/errors.h"

#if HAVE_LAME
#include <lame/lame.h>

namespace import {

EncodingTaskPtr EncodingTaskMP3::Create(const std::string& filename)
{
    return EncodingTaskPtr(new EncodingTaskMP3(filename));
}

unsigned int EncodingTaskMP3::Run()
{
    size_t totalsamples = m_input_size / 4;
    
    lame_t gfp = lame_init();

    int rc2 = lame_set_preset(gfp, STANDARD);
    if (rc2<0)
    {
	TRACE << "lame_set_preset returned " << rc2 << "\n";
	return EINVAL;
    }
    rc2 = lame_init_params(gfp);
    if (rc2<0)
    {
	TRACE << "lame_init_params returned " << rc2 << "\n";
	return EINVAL;
    }

    const int LUMP = 128*1024; // bytes

    short *sbuf = new short[LUMP/2];
    unsigned char *obuf = new unsigned char[LUMP];

    size_t samples_done = 0;
    
    FILE *f = fopen(m_output_filename.c_str(), "wb+");

    if (!f)
    {
	TRACE << "Lame encoder can't open '" << m_output_filename << "'\n";
	return (unsigned int)errno;
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
	
	rc2 = lame_encode_buffer_interleaved(gfp, sbuf, (int)nsamples, obuf,
					     LUMP);
	if (rc2 < 0)
	{
	    TRACE << "Lame encode failed: " << rc2 << "\n";
	    break;
	}

	size_t rc3 = fwrite(obuf, 1, (size_t)rc2, f);
	if (rc3 != (size_t)rc2)
	{
	    TRACE << "Lame encode write failed\n";
	}

	samples_done += nsamples;

	FireProgress((unsigned)samples_done, (unsigned)totalsamples);
    }

    int rc = lame_encode_flush(gfp, obuf, LUMP);

    rc = (int)fwrite(obuf, 1, (size_t)rc, f);
	
    lame_mp3_tags_fid(gfp, f);
    lame_close(gfp);
    fclose(f);

    delete[] sbuf;
    delete[] obuf;

    {
	util::Mutex::Lock lock(m_rename_mutex);
	m_rename_stage = LATE;
	if (!m_rename_filename.empty())
	{
	    util::RenameWithMkdir(m_output_filename.c_str(), 
				  m_rename_filename.c_str());
//	    TRACE << "Tag point 2\n";
	    import::Tags tags;
	    tags.Open(m_rename_filename);
	    tags.Write(m_rename_tags.get());
	    m_rename_filename.clear();
	}
    }

//    TRACE << "MP3 encoding finished\n";
    return 0;
}

} // namespace import

#endif // HAVE_LAME
