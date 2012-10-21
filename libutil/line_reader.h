#ifndef LINE_READER_H
#define LINE_READER_H 1

#include "stream.h"
#include <string>

namespace util {

/** Returns single lines of text from a Stream.
 *
 * Useful for HTTP headers and the like.
 */
class LineReader
{
    StreamPtr m_stream;
    size_t m_buffered;

    enum { MAX_LINE = 1024 };
    char m_buffer[MAX_LINE];

public:
    explicit LineReader(StreamPtr);

    /** Repeatedly Read()s the stream until CR, LF, EOF, or maximum.
     *
     * Returns an error (ENODATA) on EOF.
     */
    unsigned GetLine(std::string *line);

    /** Returns anything that hasn't been assembled into a line yet.
     */
    const char *GetLeftovers(size_t *nbytes);
};

} // namespace util

#endif
