#include "web.h"
#include "libutil/trace.h"
#include "libutil/string_stream.h"
#include "libutil/urlescape.h"
#include "libmediadb/db.h"
#include "libmediadb/schema.h"
#include <time.h>
#include <boost/format.hpp>

RootContentFactory::RootContentFactory(mediadb::Database *db)
    : m_db(db)
{
}

static std::string EscapeQuotes(const std::string& s)
{
    std::string result;
    result.reserve(s.length());

    for (unsigned int i=0; i<s.length(); ++i)
    {
	if (s[i] == '\"')
	    result += "\\\"";
	else
	    result += s[i];
    }
    return result;
}

static std::string Google(const std::string& search)
{
    return "<a title='Google \"" + EscapeQuotes(search) + "\"'"
	" href=\"http://www.google.com/search?q=%22" + util::URLEscape(search)
	+ "%22\"><img src=/layout/search-google.png width=16 height=16 border=0></a>";
}

static std::string Amazon(const std::string& search)
{
    return "<a title='Search Amazon for \"" + EscapeQuotes(search) + "\"'"
	" href=\"http://www.amazon.co.uk/s/?url=search-alias%3Dpopular&field-keywords=%22" + util::URLEscape(search)
	+ "%22\"><img src=/layout/search-amazon.png width=16 height=16 border=0></a>";
}

/* See http://en.wikipedia.org/w/opensearch_desc.php
 */
static std::string Wikipedia(const std::string& search)
{
    return "<a title='Search Wikipedia for \"" + EscapeQuotes(search) + "\"'"
	" href=\"http://en.wikipedia.org/w/index.php?title=Special:Search&amp;search=%22" + util::URLEscape(search)
	+ "%22\"><img src=/layout/search-wikipedia.png width=16 height=16 border=0></a>";
}

util::SeekableStreamPtr RootContentFactory::HomePageStream()
{
    util::StringStreamPtr ss = util::StringStream::Create();

    char hostname[256];
    hostname[0] = '\0';
    gethostname(hostname, sizeof(hostname));
    char *dot = strchr(hostname, '.');
    if (dot && dot > hostname)
	*dot = '\0';

    ss->str() = "<html>"
	"<meta http-equiv=Content-Type content=text/html;charset=UTF-8>"
	"<link rel=stylesheet href=/layout/default.css>"
	"<title>Chorale on ";
    ss->str() += hostname;
    ss->str() += "</title>"
	"<body><div class=head>&nbsp;<img src=/layout/icon.png width=32 height=32 align=baseline style=\"padding-top:5px\"> Chorale on ";
    ss->str() += hostname;
    ss->str() += "</div>";

    // TV/Radio listings

    ss->str() += "<table border=0 cellpadding=2 cellspacing=0><tr><th colspan=9 class=head>TV and radio listings</th></tr>"
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
    ss->str() += "<td><a href=/epg/r>All</a></td></tr></table><br><br>";

    // Recent albums

    db::QueryPtr qp = m_db->CreateQuery();
    qp->Where(qp->Restrict(mediadb::ID, db::EQ, mediadb::BROWSE_ROOT));
    db::RecordsetPtr rs = qp->Execute();
    if (rs && !rs->IsEOF())
    {
	std::string newalbums = rs->GetString(mediadb::PATH) + "/New Albums";
	qp = m_db->CreateQuery();
	qp->Where(qp->Restrict(mediadb::PATH, db::EQ, newalbums));
	rs = qp->Execute();
	std::vector<unsigned int> vec;
	if (rs && !rs->IsEOF())
	    mediadb::ChildrenToVector(rs->GetString(mediadb::CHILDREN), &vec);
	if (!vec.empty())
	{
	    ss->str() += "<table border=0 cellpadding=0 cellspacing=0><tr><th colspan=3 class=head>New albums</th></tr>";

	    for (std::vector<unsigned int>::const_iterator ci = vec.begin();
		 ci != vec.end();
		 ++ci)
	    {
		unsigned int albumid = *ci;
		
		qp = m_db->CreateQuery();
		qp->Where(qp->Restrict(mediadb::ID, db::EQ, albumid));
		rs = qp->Execute();

		if (!rs || rs->IsEOF())
		{
		    TRACE << "No sign of album " << albumid << "\n";
		    continue;
		}

		std::vector<unsigned int> tracks;
		std::string title, artist, maybeartist, arturl;
		mediadb::ChildrenToVector(rs->GetString(mediadb::CHILDREN),
					  &tracks);

		if (rs->GetInteger(mediadb::TYPE) == mediadb::PLAYLIST)
		    title = rs->GetString(mediadb::TITLE);

		TRACE << "Album " << albumid << " " << tracks.size() << " tracks\n";
		
		for (std::vector<unsigned int>::const_iterator cii = tracks.begin();
		     cii != tracks.end();
		     ++cii)
		{
		    qp = m_db->CreateQuery();
		    qp->Where(qp->Restrict(mediadb::ID, db::EQ, *cii));
		    rs = qp->Execute();

		    if (!rs || rs->IsEOF())
			continue;

		    if (rs->GetInteger(mediadb::TYPE) == mediadb::FILE
			|| rs->GetInteger(mediadb::TYPE) == mediadb::IMAGE)
		    {
			arturl = (boost::format("/content/%x") % *cii).str();
		    }

		    if (rs->GetInteger(mediadb::TYPE) != mediadb::TUNE)
			continue;

		    std::string thistitle = rs->GetString(mediadb::ALBUM);
		    if (title.empty())
			title = thistitle;

		    std::string thisartist = rs->GetString(mediadb::ARTIST);
		    if (maybeartist.empty())
			maybeartist = thisartist;
		    else if (artist.empty())
		    {
			if (maybeartist == thisartist)
			{
			    artist = maybeartist;
			}
			if (!thisartist.empty())
			    maybeartist = thisartist;
		    }
		}
		if (tracks.size() == 1)
		    artist = maybeartist;
		if (arturl.empty())
		    arturl = "/layout/noart48.png";
		if (!title.empty())
		{
		    ss->str() += "<tr><td rowspan=2><img src=" + arturl
			+ " width=48 height=48>&nbsp;</td><td valign=center><i>" + title + "</i></td><td>"
			"&nbsp;"
			+ Google(title) + Amazon(title) + Wikipedia(title)
			+ "<tr><td valign=top>";

		    if (artist.empty())
			ss->str() += "Various artists<hr><td>&nbsp;";
		    else
			ss->str() += artist + "<hr></td><td valign=top>"
			    "&nbsp;"
			    + Google(artist) + Amazon(artist) 
			    + Wikipedia(artist);

		    ss->str() += "</td></tr>";
		}
	    }
	    ss->str() += "</table>";
	}
    }

    ss->str() += "</body></html>";

    ss->Seek(0);
    return ss;
}

bool RootContentFactory::StreamForPath(const util::WebRequest *rq, 
				       util::WebResponse *rs)
{
    TRACE << "Path '" << rq->path << "'\n";
    if (rq->path == "/")
    {
	rs->ssp = HomePageStream();
	TRACE << "Home page\n";
	return true;
    }

    return false;
}
