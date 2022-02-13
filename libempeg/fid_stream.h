#ifndef LIBEMPEG_FID_STREAM_H
#define LIBEMPEG_FID_STREAM_H 1

#include "libutil/stream.h"
#include <memory>

namespace empeg {

class ProtocolClient;

/** A SeekableStream that pulls tune data from an Empeg car-player.
 *
 * Currently read-only.
 */
class FidStream: public util::SeekableStream
{
    ProtocolClient *m_client;
    unsigned int m_fid;
    unsigned int m_size;

    FidStream(ProtocolClient*, unsigned int fid, unsigned int size);

public:
    static unsigned int CreateRead(ProtocolClient*, unsigned int fid,
				   std::unique_ptr<util::Stream> *result);
    static unsigned int CreateWrite(ProtocolClient*, unsigned int fid,
				    std::unique_ptr<util::Stream> *result);

    // Being a SeekableStream
    unsigned GetStreamFlags() const { return READABLE|WRITABLE|SEEKABLE; }
    uint64_t GetLength();
    unsigned int SetLength(uint64_t);
    unsigned int ReadAt(void *buffer, uint64_t pos, size_t len, size_t *pread);
    unsigned int WriteAt(const void *buffer, uint64_t pos, size_t len, 
			 size_t *pwrote);
};

} // namespace empeg

#endif
