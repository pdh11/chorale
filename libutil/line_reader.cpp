#include "line_reader.h"
#include "trace.h"
#include "stream.h"
#include <errno.h>
#include <assert.h>
#include <string.h>

namespace util {

GreedyLineReader::GreedyLineReader(Stream *stream)
    : m_stream(stream), m_buffered(0)
{
}

unsigned GreedyLineReader::GetLine(std::string *line)
{
    for (;;)
    {
	const char *cr = (const char*)memchr(m_buffer, '\r', m_buffered);

	if (!cr || (size_t(cr-m_buffer) == m_buffered-1))
	{
	    if (m_buffered == MAX_LINE)
	    {
		*line = std::string(m_buffer, m_buffer+m_buffered);
		m_buffered = 0;
		return 0;
	    }

	    size_t nread;
	    unsigned rc = m_stream->Read(m_buffer + m_buffered,
					 MAX_LINE - m_buffered, &nread);
	    if (rc != 0)
		return rc;
	    if (nread == 0)
	    {
//		TRACE << "EOF\n";
		return EIO;
	    }
	    m_buffered += nread;
	}
	else
	{
	    *line = std::string(m_buffer, (size_t)(cr - m_buffer));
	    if (cr[1] == '\n')
		++cr;
	    ++cr;
	    size_t toskip = (size_t)(cr - m_buffer);
	    m_buffered -= toskip;
	    memmove(m_buffer, cr, m_buffered);
//	    TRACE << "Returning line, " << m_buffered << " more bytes buffered\n";
	    return 0;
	}
    }
}

void GreedyLineReader::ReadLeftovers(void *buffer, size_t n, size_t *nread)
{
    if (n > m_buffered)
	n = m_buffered;
    *nread = n;
    memcpy(buffer, m_buffer, n);
    m_buffered -= n;
    memmove(m_buffer, m_buffer+n, m_buffered);
}

} // namespace util

#ifdef TEST
# include "string_stream.h"

struct Test 
{
    const char *input;
    const char *lines[10];
};

static const Test tests[] = {
{
    "GET / HTTP/1.0\r\n"
    "\r\n",

    { "GET / HTTP/1.0",
      "",
      NULL }
}

};

static void DoTest(const Test *t)
{
    util::StringStream ss(t->input);

    util::GreedyLineReader lr(&ss);
    const char *const *ptr = &t->lines[0];

    while (*ptr)
    {
	std::string line;
	unsigned int rc = lr.GetLine(&line);
	assert(rc == 0);
	if (line != *ptr)
	{
	    TRACE << "** Fail, expected '" << *ptr << "' got '" << line
		  << "'\n";
	}
	assert(line == *ptr);
	++ptr;
    }

    char buffer;
    size_t nbytes;
    lr.ReadLeftovers(&buffer, 1, &nbytes);
    assert(nbytes == 0);
}

int main(int, char *[])
{
    for (unsigned int i=0; i<sizeof(tests)/sizeof(tests[0]); ++i)
	DoTest(&tests[i]);

    return 0;
}

#endif
