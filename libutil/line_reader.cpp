#include "line_reader.h"
#include "trace.h"
#include "string_stream.h"
#include <errno.h>

namespace util {

LineReader::LineReader(StreamPtr stream)
    : m_stream(stream), m_buffered(0)
{
}

unsigned LineReader::GetLine(std::string *line)
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
		return ENODATA;
	    }
	    m_buffered += nread;
	}
	else
	{
	    *line = std::string(m_buffer, cr - m_buffer);
	    if (cr[1] == '\n')
		++cr;
	    ++cr;
	    unsigned int toskip = cr - m_buffer;
	    m_buffered -= toskip;
	    memmove(m_buffer, cr, m_buffered);
	    return 0;
	}
    }
}

const char *LineReader::GetLeftovers(size_t *nbytes)
{
    *nbytes = m_buffered;
    m_buffered = 0;
    return m_buffer;
}

} // namespace util

#ifdef TEST

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

void DoTest(const Test *t)
{
    util::StringStreamPtr ss = util::StringStream::Create();
    ss->str() = t->input;

    util::LineReader lr(ss);
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

    size_t nbytes;
    lr.GetLeftovers(&nbytes);
    assert(nbytes == 0);
}

int main(int, char *[])
{
    for (unsigned int i=0; i<sizeof(tests)/sizeof(tests[0]); ++i)
	DoTest(&tests[i]);

    return 0;
}

#endif
