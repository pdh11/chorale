#ifndef PEEKING_LINE_READER_H
#define PEEKING_LINE_READER_H 1

#include "socket.h"
#include "line_reader.h"
#include <string>

namespace util {

class PeekingLineReader: public LineReader
{
    util::StreamSocketPtr m_socket;
    std::string m_line;

public:
    explicit PeekingLineReader(util::StreamSocketPtr);

    // Being a LineReader
    unsigned int GetLine(std::string*);
};

} // namespace util

#endif
