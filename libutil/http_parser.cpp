#include "http_parser.h"
#include "line_reader.h"
#include "trace.h"
#include <boost/tokenizer.hpp>

namespace util {

namespace http {

unsigned int Parser::GetRequestLine(std::string *verb, std::string *path,
					  std::string *version)
{
    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
    boost::char_separator<char> sep(" \t");

    std::string line;
    unsigned int rc = m_line_reader->GetLine(&line);
    if (rc)
	return rc;

    tokenizer bt(line, sep);
    tokenizer::iterator bti = bt.begin();
    tokenizer::iterator end = bt.end();

//    TRACE << "Request " << line << "\n";

    if (bti == end)
    {
	TRACE << "Don't like request line '" << line << "'\n";
	return EINVAL;
    }
    *verb = *bti;
    ++bti;
    if (bti == end)
    {
	TRACE << "Don't like request line '" << line << "'\n";
	return EINVAL;
    }
    *path = *bti;
    ++bti;
    if (bti == end)
    {
	TRACE << "Don't like request line '" << line << "'\n";
	return EINVAL;
    }
    if (version)
	*version = *bti;
    return 0;
}

unsigned int Parser::GetResponseLine(unsigned int *status, 
				     std::string *version)
{
    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
    boost::char_separator<char> sep(" \t");

    std::string line;
    unsigned int rc = m_line_reader->GetLine(&line);
    if (rc)
	return rc;

    tokenizer bt(line, sep);
    tokenizer::iterator bti = bt.begin();
    tokenizer::iterator end = bt.end();

//    TRACE << "Response " << line << "\n";

    if (bti == end)
    {
	TRACE << "Don't like response line '" << line << "'\n";
	return EINVAL;
    }
    if (version)
	*version = *bti;
    ++bti;
    if (bti == end)
    {
	TRACE << "Don't like response line '" << line << "'\n";
	return EINVAL;
    }
    *status = atoi(bti->c_str());
    return 0;
}

unsigned int Parser::GetHeaderLine(std::string *pkey, std::string *pvalue)
{
    std::string line;
    unsigned int rc = m_line_reader->GetLine(&line);
    if (rc)
	return rc;

    pkey->clear();
    pvalue->clear();

    std::string::size_type colon = line.find(':');
    if (colon != std::string::npos)
    {
	std::string key(line, 0, colon);
	std::string value(line, colon+1);
	std::string::size_type nonws = value.find_first_not_of(" \t");
	if (nonws != std::string::npos)
	    value = std::string(value, nonws);
	*pkey = key;
	*pvalue = value;
    }

    return 0;
}

} // namespace http

} // namespace util
