#include "web.h"
#include "libutil/trace.h"
#include "libutil/string_stream.h"
#include <time.h>
#include <boost/format.hpp>

static util::SeekableStreamPtr HomePageStream()
{
    util::StringStreamPtr ss = util::StringStream::Create();

    char hostname[256];
    hostname[0] = '\0';
    gethostname(hostname, sizeof(hostname));
    char *dot = strchr(hostname, '.');
    if (dot && dot > hostname)
	*dot = '\0';

    ss->str() = "<html>"
	"<link rel=stylesheet href=/layout/default.css>"
	"<body><div class=head>&nbsp;<img src=/layout/icon.png width=32 height=32 align=baseline style=\"padding-top:5px\"> Chorale on ";
    ss->str() += hostname;
    ss->str() += "</div>"
	"<table border=0 cellpadding=2 cellspacing=0><tr><th colspan=9 class=head>TV and radio listings</th></tr>"
	"<tr><td align=right>TV:</td>"
	"<td><a href=/epg/t0>Today</a></td>"
	"<td><a href=/epg/t1>Tomorrow</a></td>";
    for (unsigned int i=2; i<7; ++i)
    {
	time_t day = time(NULL) + i * 86400;
	struct tm stm;
	localtime_r(&day, &stm);
	char buffer[80];
	strftime(buffer, sizeof(buffer), "%a", &stm);
	ss->str() += (boost::format("<td><a href=/epg/t%u>%s</a></td>")
		      % i % buffer).str();
    }
    ss->str() += "<td><a href=/epg/t>All</a></td></tr>"
	"<tr><td align=right>Radio:</td>"
	"<td><a href=/epg/r0>Today</a></td>"
	"<td><a href=/epg/r1>Tomorrow</a></td>";
    for (unsigned int i=2; i<7; ++i)
    {
	time_t day = time(NULL) + i * 86400;
	struct tm stm;
	localtime_r(&day, &stm);
	char buffer[80];
	strftime(buffer, sizeof(buffer), "%a", &stm);
	ss->str() += (boost::format("<td><a href=/epg/r%u>%s</a></td>")
		      % i % buffer).str();
    }
    ss->str() += "<td><a href=/epg/r>All</a></td></tr></table>";

    ss->str() += "</body></html>";

    ss->Seek(0);
    return ss;
}

util::SeekableStreamPtr RootContentFactory::StreamForPath(const char *path)
{
    if (!strcmp(path, "/"))
	return HomePageStream();

    return util::SeekableStreamPtr();
}
