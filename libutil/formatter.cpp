#include "formatter.h"
#include <stdlib.h>
#include "trace.h"

namespace util {

Formatter::Formatter()
{
}

Formatter::~Formatter()
{
}

void Formatter::AddField(char c, const std::string& s)
{
    m_map[c] = s;
}

std::string Formatter::Format(const std::string& t)
{
    std::string result;

    const char *p = t.c_str();

    while (*p)
    {
	if (*p == '%')
	{
	    bool zeroes = false;
	    unsigned int width = 0;

	    ++p;
	    if (*p == '\0')
		break;
	    else if (*p == '%')
	    {
		result += '%';
		++p;
	    }
	    else
	    {
		if (*p == '0')
		{
		    zeroes = true;
		    ++p;
		}
		if (isdigit(*p))
		{
		    char *pp;
		    width = (unsigned int) strtoul(p, &pp, 10);
		    p = pp;
		}
		if (*p == '\0')
		    break;
		std::string s = m_map[*p];
		if (width > s.length())
		{
		    std::string pad(width - s.length(),
				    zeroes ? '0' : ' ');
		    s = pad + s;
		}
		result += s;
		++p;
	    }
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

#include <assert.h>

int main(int, char**)
{
    util::Formatter f;
    f.AddField('v', "Carrot");
    assert(f.Format("Today's vegetable is %v") == "Today's vegetable is Carrot");
    f.AddField('n', "2");

    assert(f.Format("%n,%2n,%02n") == "2, 2,02");
    return 0;
}

#endif
