#include "config.h"
#include "web_epg.h"
#include "epg.h"
#include "epg_database.h"
#include "dvb.h"
#include "dvb_service.h"
#include "recording.h"
#include "libdb/query.h"
#include "libdb/recordset.h"
#include "libutil/trace.h"
#include "libutil/string_stream.h"
#include "libutil/printf.h"
#include "libutil/counted_pointer.h"
#include <stdio.h>

#if HAVE_DVB

namespace tv {

WebEPG::WebEPG()
    : m_db(NULL),
      m_ch(NULL),
      m_dvb_service(NULL)
{
}

unsigned int WebEPG::Init(util::http::Server *ws, db::Database *db,
			  dvb::Channels *ch, dvb::Service *dvb_service)
{
    m_db = db;
    m_ch = ch;
    m_dvb_service = dvb_service;
    ws->AddContentFactory("/epg", this);
    return 0;
}

struct WebEPGItem
{
    unsigned int id;
    time_t start;
    time_t end;
    unsigned int state;
    std::string title;
};

struct WebEPGItemLess
{
    bool operator()(const WebEPGItem& e1, const WebEPGItem& e2) const
    {
	return e1.start < e2.start;
    }
};

typedef std::set<WebEPGItem, WebEPGItemLess> epgset_t;

static const char *ColourForState(unsigned int state)
{
    switch (state)
    {
    case tv::epg::TORECORD:
	return "#ccffcc";
	break;
    case tv::epg::CLASHING:
	return "#ffcccc";
	break;
    case tv::epg::RECORDING:
	return "#ccffcc";
	break;
    case tv::epg::RECORDED:
	return "#ccffcc";
    }
    return NULL;
}

std::string WebEPG::FullItem(unsigned int id, const std::string& title,
			     const std::string& desc, unsigned int state)
{
    const char *colour = "";
    const char *states = NULL;
    enum { RECORD=1, OVERRIDE=2, CANCEL=4, RECORD_ALL=8 };
    unsigned int actions = RECORD | RECORD_ALL;
    switch (state)
    {
    case tv::epg::TORECORD:
	colour="#ccffcc";
	states="Will record";
	actions=CANCEL;
	break;
    case tv::epg::CLASHING:
	colour="#ffcccc";
	states="Can't record";
	actions=OVERRIDE;
	break;
    case tv::epg::RECORDING:
	colour="#ccffcc";
	states="Now recording";
	actions=CANCEL;
	break;
    case tv::epg::RECORDED:
	colour="#ccffcc";
	states="Recorded";
	break;
    }

    std::string s = util::SPrintf(
	"<div style=\"background-color: %s\"><font size=-2><b>%s</b> %s<br>",
	colour, title.c_str(), desc.c_str());

    if (states)
    {
	s += "<i>";
	s += states;
	s += "</i><br>";
    }
    else
	s += "<br>";

    if (actions & RECORD)
	s += util::Printf() << "<a href=\"javascript:r(" << id
			    << ")\">Record</a><br>";
    if (actions & CANCEL)
	s += util::Printf() << "<a href=\"javascript:ca(" << id
			    << ")\">Cancel</a><br>";
    if (actions & OVERRIDE)
	s += util::Printf() << "<a href=\"javascript:ov(" << id
			    << ")\">Override!</a><br>";
    s += "</font></div>";

    TRACE << "Replacement HTML is " << s << "\n";
    return s;
}

std::unique_ptr<util::Stream> WebEPG::EPGStream(bool tv, bool radio, int day)
{
    char datebuf[80];

    time_t starttime = time(NULL);
    starttime -= (starttime % 86400);

    if (day != -1)
	starttime += day*(time_t)86400;

    time_t endtime = starttime + 86400;

    // For one-day listings, go through to 6am the next day
    if (day != -1)
	endtime += (time_t)6*60*60;

    std::string s =
	"<html>"
	"<meta http-equiv=Content-Type content=text/html;charset=UTF-8>"
	"<link rel=stylesheet href=/layout/default.css>"
	"<script>"
	"function expand(id) { var request = new XMLHttpRequest; "
	" request.open(\"GET\", \"/epg/expand/\" + id, false); "
	" request.send(\"\");"
	" document.all[\"e\"+id].innerHTML = request.responseText;"
	"};"
	"function r(id) { var request = new XMLHttpRequest; "
	" request.open(\"GET\", \"/epg/record/\" + id, false); "
	" request.send(\"\");"
	" document.all[\"e\"+id].innerHTML = request.responseText;"
	"};"
	"function ca(id) { var request = new XMLHttpRequest; "
	" request.open(\"GET\", \"/epg/cancel/\" + id, false); "
	" request.send(\"\");"
	" document.all[\"e\"+id].innerHTML = request.responseText;"
	"}"
	"function ov(id) { var request = new XMLHttpRequest; "
	" request.open(\"GET\", \"/epg/override/\" + id, false); "
	" request.send(\"\");"
	" document.all[\"e\"+id].bgColor = \"\";"
	"}"
	"</script>"
	"<body>"
	"<div class=head>&nbsp;<a href=/"
	" onmouseover='document.images[0].src=\"/layout/iconsel.png\"'"
	" onmouseout='document.images[0].src=\"/layout/icon.png\"'>"
	"<img border=0 src=/layout/icon.png width=32 height=32 align=baseline style=\"padding-top:5px\"></a>";
    if (!radio)
	s += " TV";
    else if (!tv)
	s += " Radio";
    s +=" Listings";

    if (day != -1)
    {
	struct tm tm;
	localtime_r(&starttime, &tm);
	strftime(datebuf, sizeof(datebuf), " for %A %F", &tm);
	s += datebuf;
    }
    
    s += "</div>"
	"<table border=0 cellspacing=0 cellpadding=0>";

    bool rowtint = true;

    std::vector<bool> wanted;
    wanted.resize(m_ch->Count());
    for (unsigned int i=0; i<wanted.size(); ++i)
    {
	const dvb::Channel& ch = m_ch->begin()[i];
	if (ch.videopid != 0)
	    wanted[i] = tv;
	else if (ch.audiopid != 0)
	    wanted[i] = radio;
	else
	    wanted[i] = false;
    }

    for (;;)
    {
	db::QueryPtr qp(m_db->CreateQuery());
	qp->Where(qp->And(qp->Restrict(tv::epg::START, db::GE, 
				       (uint32_t)starttime),
			  qp->Restrict(tv::epg::START, db::LT, 
				       (uint32_t)endtime)));
	db::RecordsetPtr rs = qp->Execute();

	if (!rs || rs->IsEOF())
	    break;

	std::vector<epgset_t> vec;
	vec.resize(m_ch->Count());

	while (!rs->IsEOF())
	{
	    unsigned int channel = rs->GetInteger(tv::epg::CHANNEL);
	    if (wanted[channel])
	    {
		WebEPGItem wei;
		wei.id = rs->GetInteger(tv::epg::ID);
		wei.start = rs->GetInteger(tv::epg::START);
		wei.end = rs->GetInteger(tv::epg::END);
		wei.title = rs->GetString(tv::epg::TITLE);
		wei.state = rs->GetInteger(tv::epg::STATE);
		vec[channel].insert(wei);
	    }
	    rs->MoveNext();
	}

	struct tm tm;
	localtime_r(&starttime, &tm);
	strftime(datebuf, sizeof(datebuf), "%A %F", &tm);

	s += "<tr>";

	bool coltint = rowtint;

	unsigned int k=0;
	for (dvb::Channels::const_iterator i = m_ch->begin();
	     i != m_ch->end();
	     ++i, ++k)
	{
	    if (wanted[k])
	    {
		s += (coltint ? "<th>" : "<th class=alt>") + i->name;
		if (day == -1)
		    s += std::string("<br>") + datebuf;
		
		s += "</th>";
		coltint = !coltint;
	    }
	}

	s += "</tr><tr>";

	coltint = rowtint;

	for (unsigned int i=0; i<vec.size(); ++i)
	{
	    if (wanted[i])
	    {
		s += coltint ? "<td valign=top>" : "<td valign=top class=alt>";

		s += "<table border=0>";

		bool donedate = false;
		const epgset_t& epgs = vec[i];
		for (epgset_t::const_iterator j = epgs.begin(); j != epgs.end();
		     ++j)
		{
		    struct tm stm;
		    localtime_r(&j->start, &stm);
		    const char *colour = ColourForState(j->state);
		    const char *maybedate = "";
		    if ((j->start - starttime) >= 86400
			&& !donedate)
		    {
			strftime(datebuf, sizeof(datebuf), "<br>%a", &stm);
			maybedate = datebuf;
			donedate = true;
		    }
		    if (colour)
		    {
			s += util::SPrintf(
			    "<tr><td valign=top><font size=-2><b>%02u:%02u%s</b></font></td>"
			    "<td id=\"e%u\"><div style=\"background-color: %s\"><font size=-2>%s<a href=\"javascript:expand(%u)\">[+]</a>"
			    "</font></div>"
			    "</td></tr>",
			    stm.tm_hour,
			    stm.tm_min,
			    maybedate,
			    j->id,
			    colour,
			    j->title.c_str(),
			    j->id);
		    }
		    else
		    {
			s += util::SPrintf(
			    "<tr><td valign=top><font size=-2><b>%02u:%02u%s</b></font></td>"
			    "<td id=\"e%u\"><font size=-2>%s<a href=\"javascript:expand(%u)\">[+]</a>"
			    "</font>"
			    "</td></tr>",
			    stm.tm_hour,
			    stm.tm_min,
			    maybedate,
			    j->id,
			    j->title.c_str(),
			    j->id);
		    }
		}
		s += "</table></td>";
		
		coltint = !coltint;
	    }
	}

	s += "</tr>";

	if (day != -1)
	    break;

	starttime += 86400;
	endtime += 86400;
	rowtint = !rowtint;
    }

    s += "</table></body></html>";
    return std::unique_ptr<util::Stream>(new util::StringStream(s));
}

std::unique_ptr<util::Stream> WebEPG::ExpandStream(unsigned int id)
{
    std::string s;
    db::QueryPtr qp = m_db->CreateQuery();
    qp->Where(qp->Restrict(tv::epg::ID, db::EQ, id));
    db::RecordsetPtr rs = qp->Execute();
    if (!rs || rs->IsEOF())
    {
	s = "<td><font color=red size=-2>Error</font></td>";
    }
    else
    {
	s = FullItem(id, rs->GetString(tv::epg::TITLE),
		     rs->GetString(tv::epg::DESCRIPTION),
		     rs->GetInteger(tv::epg::STATE));
    }
    return std::unique_ptr<util::Stream>(new util::StringStream(s));
}

std::unique_ptr<util::Stream> WebEPG::RecordStream(unsigned int id)
{
    std::string s;
    db::QueryPtr qp = m_db->CreateQuery();
    qp->Where(qp->Restrict(tv::epg::ID, db::EQ, id));
    db::RecordsetPtr rs = qp->Execute();
    if (!rs || rs->IsEOF())
    {
	s = "<td><font color=red size=-2>Error</font></td>";
    }
    else
    {
	m_dvb_service->Record(rs->GetInteger(tv::epg::CHANNEL),
			      rs->GetInteger(tv::epg::START),
			      rs->GetInteger(tv::epg::END),
			      rs->GetString(tv::epg::TITLE));
								    
	rs->SetInteger(tv::epg::STATE, tv::epg::TORECORD);
	rs->Commit();
	s = FullItem(id, rs->GetString(tv::epg::TITLE),
		     rs->GetString(tv::epg::DESCRIPTION),
		     rs->GetInteger(tv::epg::STATE));
    }
    return std::unique_ptr<util::Stream>(new util::StringStream(s));
}

std::unique_ptr<util::Stream> WebEPG::CancelStream(unsigned int id)
{
    std::string s;
    db::QueryPtr qp = m_db->CreateQuery();
    qp->Where(qp->Restrict(tv::epg::ID, db::EQ, id));
    db::RecordsetPtr rs = qp->Execute();
    if (!rs || rs->IsEOF())
    {
	s = "<td><font color=red size=-2>Error</font></td>";
    }
    else
    {
	rs->SetInteger(tv::epg::STATE, tv::epg::NONE);
	rs->Commit();
	s = FullItem(id, rs->GetString(tv::epg::TITLE),
		     rs->GetString(tv::epg::DESCRIPTION),
		     rs->GetInteger(tv::epg::STATE));
    }
    return std::unique_ptr<util::Stream>(new util::StringStream(s));
}

bool WebEPG::StreamForPath(const util::http::Request *rq, 
			   util::http::Response *rs)
{
    unsigned int id;

    const char *path = rq->path.c_str();

    if (strncmp(path, "/epg", 4) != 0)
	return false;

    if (!strcmp(path, "/epg"))
    {
	rs->body_source = EPGStream(true, true, -1);
    }
    else if (sscanf(path, "/epg/expand/%u", &id) == 1)
    {
	rs->body_source = ExpandStream(id);
    }
    else if (sscanf(path, "/epg/record/%u", &id) == 1)
    {
	rs->body_source = RecordStream(id);
    }
    else if (sscanf(path, "/epg/cancel/%u", &id) == 1)
    {
	rs->body_source = CancelStream(id);
    }
    else if (sscanf(path, "/epg/r%u", &id) == 1)
	rs->body_source = EPGStream(false, true, (int)id);
    else if (sscanf(path, "/epg/t%u", &id) == 1)
	rs->body_source = EPGStream(true, false, (int)id);
    else if (!strcmp(path, "/epg/r"))
	rs->body_source = EPGStream(false, true, -1);
    else if (!strcmp(path, "/epg/t"))
	rs->body_source = EPGStream(true, false, -1);
    else if (!strncmp(path, "/epg/", 5))
    {
	rs->body_source.reset(
	    new util::StringStream("<i>niddle noddle noo</i>"));
    } else {
        return false;
    }

    return true;
}

} // namespace tv

#endif // HAVE_DVB
