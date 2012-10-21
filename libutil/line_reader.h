#ifndef LINE_READER_H
#define LINE_READER_H 1

#include <string>

namespace util {

class Stream;

/** Returns single lines of text from a Stream.
 *
 * Useful for HTTP headers and the like.
 */
class LineReader
{
public:
    virtual ~LineReader() {}

    /** Returns exactly one line of input.
     *
     * Returns an error (ENODATA) on EOF.
     */
    virtual unsigned GetLine(std::string *line) = 0;
};

/** A LineReader which assumes that it can read the whole stream, but knows
 * nothing about the actual type of the stream.
 */
class GreedyLineReader: public LineReader
{
    Stream *m_stream;
    size_t m_buffered;

    enum { MAX_LINE = 1024 };
    char m_buffer[MAX_LINE];

public:
    explicit GreedyLineReader(Stream*);

    /** Repeatedly Read()s the stream until CR, LF, EOF, or maximum.
     */
    unsigned GetLine(std::string *line);

    /** Returns anything that hasn't been assembled into a line yet.
     *
     * Because this is a greedy line-reader, and can't peek ahead in
     * the stream, there might be quite a lot of this (up to nearly
     * MAX_LINE bytes).
     */
    void ReadLeftovers(void *buffer, size_t n, size_t *nread);
};

} // namespace util

#endif
