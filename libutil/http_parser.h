#ifndef LIBUTIL_HTTP_PARSER_H
#define LIBUTIL_HTTP_PARSER_H 1

#include <string>

namespace util {

class LineReader;

namespace http {

class Parser
{
    LineReader *m_line_reader;

public:
    explicit Parser(LineReader *line_reader) : m_line_reader(line_reader) {}

    unsigned int GetRequestLine(std::string *verb, std::string *path,
				std::string *version);
    unsigned int GetResponseLine(unsigned int *status, std::string *version);

    /** Returns an empty "key" and no error for final, blank line.
     */
    unsigned int GetHeaderLine(std::string *key, std::string *value);
};

} // namespace http

} // namespace util

#endif
