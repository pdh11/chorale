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

    StringStream();
    explicit StringStream(const std::string&);
    ~StringStream();

public:    
    typedef boost::intrusive_ptr<StringStream> StringStreamPtr;

    static StringStreamPtr Create();
    static StringStreamPtr Create(const std::string&);

    // Being a SeekableStream
    unsigned ReadAt(void *buffer, pos64 pos, size_t len, size_t *pread);
    unsigned WriteAt(const void *buffer, pos64 pos, size_t len, 
		     size_t *pwrote);
    pos64 GetLength();
    unsigned SetLength(pos64);

    std::string& str() { return m_string; }
};

typedef boost::intrusive_ptr<StringStream> StringStreamPtr;

} // namespace util

#endif
