#include "file_stream.h"
#include "file_stream_posix.h"
#include "file_stream_win32.h"

namespace util {

#ifdef WIN32
using win32::FileStream;
#else
using posix::FileStream;
#endif

unsigned int OpenFileStream(const char *filename, FileMode mode,
			    SeekableStreamPtr *pstm)
{
    FileStream *f = new FileStream();

    unsigned int rc = f->Open(filename, mode);
    if (rc)
    {
	delete f;
	return rc;
    }

    *pstm = SeekableStreamPtr(f);

    return 0;
}

} // namespace util
