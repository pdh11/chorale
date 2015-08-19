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
    unsigned GetStreamFlags() const override
    {
        return READABLE|WRITABLE|SEEKABLE;
    }

    unsigned ReadAt(void *buffer, uint64_t pos, size_t len,
                    size_t *pread) override;
    unsigned WriteAt(const void *buffer, uint64_t pos, size_t len, 
		     size_t *pwrote) override;
    uint64_t GetLength() override;
    unsigned SetLength(uint64_t) override;

    std::string& str() { return m_string; }
};

} // namespace util

#endif
