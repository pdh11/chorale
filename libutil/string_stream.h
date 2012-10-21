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
    std::string::size_type m_pos;

    StringStream();
    ~StringStream();

public:    
    typedef boost::intrusive_ptr<StringStream> StringStreamPtr;

    static StringStreamPtr Create();

    // Being a SeekableStream
    unsigned Read(void *buffer, size_t len, size_t *pread);
    unsigned Write(const void *buffer, size_t len, size_t *pwrote);
    void Seek(pos64 pos);
    pos64 Tell();
    pos64 GetLength();
    unsigned SetLength(pos64);

    std::string& str() { return m_string; }
};

typedef boost::intrusive_ptr<StringStream> StringStreamPtr;

} // namespace util

#endif
