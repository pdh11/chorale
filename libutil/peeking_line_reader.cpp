#include "peeking_line_reader.h"
#include "socket.h"
#include "trace.h"
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>

namespace util {

PeekingLineReader::PeekingLineReader(StreamSocketPtr socket)
    : m_socket(socket)
{
}

unsigned int PeekingLineReader::GetLine(std::string *line)
{
    char buf[1024];
    size_t nread;

    for (;;)
    {
	unsigned int rc = m_socket->ReadPeek(buf, sizeof(buf), &nread);
	if (rc != 0)
	{
//	    TRACE << "ReadPeek gave error " << rc << "\n";
	    return rc;
	}
	if (nread == 0)
	{
//	    TRACE << "ReadPeek gave EOF\n";
	    return EIO;
	}

//	TRACE << "Peeked " << nread << " bytes\n";

	char *cr = (char*)memchr(buf, '\n', nread);
	if (cr)
	{
	    nread = (cr-buf)+1;
//	    TRACE << "Found CR at pos " << nread << "\n";
	}

	rc = m_socket->ReadAll(buf, nread);

	m_line.append(buf, nread);

	if (cr)
	    break;
    }

    if (m_line.length() >= 2)
	m_line.erase(m_line.length()-2); // CR LF
    
//    TRACE << "PLR returns '" << m_line << "'\n";

    *line = m_line;
    m_line.clear();
    return 0;
}

} // namespace util
