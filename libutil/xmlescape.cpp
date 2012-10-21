#include "xmlescape.h"
#include <assert.h>
#include <string.h>

namespace util {

std::string XmlEscape(const std::string& s)
{
    std::string result;
    result.reserve(s.length());

    for (unsigned int i=0; i<s.length(); ++i)
    {
	unsigned char c = (unsigned char)s[i];
	if (c == '&')
	    result += "&amp;";
	else if (c == '\"')
	    result += "&quot;";
	else if (c == '<')
	    result += "&lt;";
	else if (c == '>')
	    result += "&gt;";
	else if (c >= ' ' || c == 10) // No control characters please
	    result += (char)c;
    }
    return result;
}

std::string XmlUnEscape(const std::string& s)
{
    std::string result;
    result.reserve(s.length());

    const char *p = s.c_str();
    while (*p)
    {
	if (*p == '&')
	{
	    const char *p2 = strchr(p, ';');
	    if (!p2)
		return result;
	    if (p2 == p+5 && !strncmp(p+1, "quot", 4))
		result += '\"';
	    else if (p2 == p+4 && !strncmp(p+1, "amp", 3))
		result += '&';
	    else if (p2 == p+3)
	    {
		if (!strncmp(p+1, "lt", 2))
		    result += "<";
		else if (!strncmp(p+1, "gt", 2))
		    result += ">";
	    }
	    p = p2+1;
	}
	else
	{
	    result += *p;
	    ++p;
	}
    }
    return result;
}

} // namespace util

#ifdef TEST

int main()
{
    assert(util::XmlEscape("foo") == "foo");
    assert(util::XmlEscape("\"hi\"") == "&quot;hi&quot;");
    assert(util::XmlEscape("X&Y") == "X&amp;Y");
    assert(util::XmlEscape("X<>Y") == "X&lt;&gt;Y");
    assert(util::XmlEscape("Beyonc\xC3\xA9 foo") == "Beyonc\xC3\xA9 foo");
    assert(util::XmlEscape("You\xE2\x80\x99re My Flame.flac")
	   == "You\xE2\x80\x99re My Flame.flac");

    assert(util::XmlUnEscape("foo") == "foo");
    assert(util::XmlUnEscape("&quot;hi&quot;") == "\"hi\"");
    assert(util::XmlUnEscape("&amp;quot;hi&amp;quot;") == "&quot;hi&quot;");
    assert(util::XmlUnEscape("X&amp;Y") == "X&Y");
    assert(util::XmlUnEscape("X&lt;&gt;Y") == "X<>Y");
    assert(util::XmlUnEscape("Beyonc\xC3\xA9 foo") == "Beyonc\xC3\xA9 foo");
    assert(util::XmlUnEscape("You\xE2\x80\x99re My Flame.flac")
	   == "You\xE2\x80\x99re My Flame.flac");

    return 0;
}

#endif
