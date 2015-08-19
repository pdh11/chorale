#include "file_stream.h"
#include "file_stream_posix.h"
#include "file_stream_win32.h"
#include "counted_pointer.h"

namespace util {

#ifdef WIN32
using win32::FileStream;
#else
using posix::FileStream;
#endif

unsigned int OpenFileStream(const char *filename, unsigned int mode,
			    std::unique_ptr<Stream> *pstm)
{
    FileStream *f = new FileStream();

    unsigned int rc = f->Open(filename, mode);
    if (rc)
    {
	delete f;
	return rc;
    }

    pstm->reset(f);

    return 0;
}

} // namespace util

#ifdef TEST
# include "stream_test.h"

int main()
{
    std::unique_ptr<util::Stream> msp;

    unsigned int rc = util::OpenFileStream("test2.tmp", util::TEMP, &msp);
    assert(rc == 0);

    TestSeekableStream(msp.get());

    return 0;
}

#endif
