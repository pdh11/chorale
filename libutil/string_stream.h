#ifndef STRING_STREAM_H
#define STRING_STREAM_H

#include <string>
#include "stream.h"

namespace util {

/** A SeekableStream backed by a std::string.
 */
class StringStream: public SeekableStream
{
    std::string m_string;

public:    
    StringStream();
    explicit StringStream(const std::string&);
    ~StringStream();

    // Being a SeekableStream
    unsigned GetStreamFlags() const { return READABLE|WRITABLE|SEEKABLE; }
    unsigned ReadAt(void *buffer, uint64_t pos, size_t len, size_t *pread);
    unsigned WriteAt(const void *buffer, uint64_t pos, size_t len, 
		     size_t *pwrote);
    uint64_t GetLength();
    unsigned SetLength(uint64_t);

    std::string& str() { return m_string; }
};

} // namespace util

#endif
