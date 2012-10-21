#include "xml.h"
#include "xmlescape.h"
#include "trace.h"
#include "stream_test.h"
#include <string.h>

namespace xml {

unsigned int SaxParser::Parse(util::StreamPtr s)
{
    enum { BUFSIZE = 1024 };
    char buffer[BUFSIZE+1];
    size_t bufused = 0;
    bool eof = false;
    unsigned int rc = 0;
    std::string tagname;
    std::string attrname;
    
    enum { CONTENT, TAG, IN_TAG, IN_ATTR, SEEK_GT, ATTR_SEEKEQ,
	   ATTR_SEEKVALUE, IN_QUOTEDVALUE, IN_VALUE } state = CONTENT;

    for (;;)
    {
	if (bufused < BUFSIZE && !eof)
	{
	    size_t nread = 0;
	    rc = s->Read(buffer+bufused, BUFSIZE-bufused, &nread);
	    if (rc)
	    {
		TRACE << "Read gave error " << rc << "\n";
		return rc;
	    }
	    if (nread == 0)
		eof = true;
	    bufused += (unsigned)nread;
	}

	if (bufused == 0 && eof)
	    return 0; // Done

	buffer[bufused] = '\0';

//	TRACE << "state " << (int)state << " buffer " << bufused << " "
//	      << buffer << "\n";

	switch (state)
	{
	case CONTENT:
	{
	    char *lt = strchr(buffer, '<');
	    if (lt == NULL)
	    {
		// xmlunescape
		std::string unesc = util::XmlUnEscape(buffer);
		m_observer->OnContent(unesc.c_str());
		bufused = 0;
	    }
	    else
	    {
		*lt = '\0';
		if (lt > buffer)
		{
		    // xmlunescape
		    std::string unesc = util::XmlUnEscape(buffer);
		    m_observer->OnContent(unesc.c_str());
		}
		state = TAG;
		unsigned int usedup = (unsigned)(lt+1 - buffer);
//		TRACE << "used up " << usedup << "\n";
		memmove(buffer, lt+1, bufused-usedup);
		bufused -= usedup;
	    }
	    break;
	}
	case TAG:
	{
	    bool end = false;
	    size_t skip = 0;
	    if (*buffer == '/')
	    {
		end = true;
		skip = 1;
	    }
	    size_t taglen = strcspn(buffer + skip, " \t\r\n/>") + skip;
	    if (taglen == bufused && !eof && bufused < BUFSIZE)
		continue;
	    char c = buffer[taglen];
	    buffer[taglen] = '\0';
	    if (end)
	    {
		m_observer->OnEnd(buffer+1);
		if (c == '>')
		    state = CONTENT;
		else
		    state = SEEK_GT;
	    }
	    else
	    {
		tagname = buffer;
		m_observer->OnBegin(buffer);
		if (c == '/')
		{
		    m_observer->OnEnd(buffer);
		    state = SEEK_GT;
		}
		else if (c == '>')
		    state = CONTENT;
		else
		    state = IN_TAG;
	    }
	    memmove(buffer, buffer+taglen+1, bufused-taglen-1);
	    bufused -= (unsigned)(taglen+1);
	    break;
	}
	
	case SEEK_GT:
	{
	    char *gt = strchr(buffer, '>');
	    if (gt == NULL && !eof && bufused < BUFSIZE)
		continue;
	    unsigned int usedup = (unsigned)(gt+1 - buffer);
	    memmove(buffer, gt+1, bufused-usedup);
	    bufused -= usedup;
	    state = CONTENT;
	    break;
	}

	// <tag  attr=thing>
	//     ^
	case IN_TAG:
	{
	    size_t skip = strspn(buffer, " \t\n\r");
	    if (skip == bufused)
	    {
		bufused = 0;
		break;
	    }
	    char c = buffer[skip];
	    if (c == '>')
		state = SEEK_GT;
	    else if (c == '/')
	    {
		m_observer->OnEnd(tagname.c_str());
		++skip;
	    }
	    else
		state = IN_ATTR;
	    memmove(buffer, buffer+skip, bufused - skip);
	    bufused -= skip;
	    break;
	}

	// <tag  attr=thing>
	//       ^
	case IN_ATTR:
	{
	    size_t attrlen = strcspn(buffer, " \t\n\r=/>");
	    if (attrlen == bufused && !eof && bufused < BUFSIZE)
		continue;
	    char c = buffer[attrlen];
	    buffer[attrlen] = '\0';
	    attrname = buffer;
	    if (c == '=')
		state = ATTR_SEEKVALUE;
	    else if (c == '>')
	    {
		m_observer->OnAttribute(buffer, "");
		state = CONTENT;
	    }
	    else if (c == '/')
	    {
		m_observer->OnAttribute(buffer, "");
		m_observer->OnEnd(tagname.c_str());
		state = SEEK_GT;
	    }
	    else
		state = ATTR_SEEKEQ;

	    unsigned int usedup = (unsigned)(attrlen+1);
	    memmove(buffer, buffer+usedup, bufused-usedup);
	    bufused -= usedup;
	    break;
	}

	// <tag attr = thing/>
	//          ^
	//
	// XML is not supposed to allow whitespace before "=", but ASX files
	// very often have it, and (as "=" isn't a valid attribute-name
	// character) it isn't ambiguous.
	case ATTR_SEEKEQ:
	{
	    size_t skip = strspn(buffer, " \t\n\r");
	    if (skip == bufused)
	    {
		bufused = 0;
		break;
	    }
	    char c = buffer[skip];
	    if (c == '=')
	    {
		state = ATTR_SEEKVALUE;
		++skip;
	    }
	    else
	    {
		m_observer->OnAttribute(attrname.c_str(), "");
		state = IN_TAG;
	    }

	    memmove(buffer, buffer+skip, bufused-skip);
	    bufused -= skip;
	    break;
	}

	// <tag attr = "thing"/>
	//            ^
	case ATTR_SEEKVALUE:
	{
	    size_t skip = strspn(buffer, " \t\n\r");
	    if (skip == bufused)
	    {
		bufused = 0;
		break;
	    }
	    char c = buffer[skip];
	    if (c == '\"')
	    {
		++skip;
		state = IN_QUOTEDVALUE;
	    }
	    else
		state = IN_VALUE;

	    memmove(buffer, buffer+skip, bufused - skip);
	    bufused -= skip;
	    break;
	}

	// <tag attr = "thing"/>
	//              ^
	case IN_QUOTEDVALUE:
	{
	    char *quote = strchr(buffer, '"');
	    if (!quote && !eof && bufused < BUFSIZE)
		continue;
	    *quote = '\0';
	    
	    std::string value = util::XmlUnEscape(buffer);
	    m_observer->OnAttribute(attrname.c_str(), value.c_str());
	    
	    state = IN_TAG;
	    unsigned int usedup = (unsigned)(quote+1 - buffer);
	    memmove(buffer, quote+1, bufused - usedup);
	    bufused -= usedup;
	    break;
	}

	// <tag attr = thing/>
	//             ^
	case IN_VALUE:
	{
	    size_t valuelen = strcspn(buffer, " \t\r\n/>");
	    if (valuelen == bufused && !eof && bufused < BUFSIZE)
		continue;

	    char c = buffer[valuelen];
	    buffer[valuelen] = '\0';
	    m_observer->OnAttribute(attrname.c_str(), 
				    util::XmlUnEscape(buffer).c_str());
	    buffer[valuelen] = c;
	    memmove(buffer, buffer+valuelen, bufused - valuelen);
	    bufused -= valuelen;
	    state = IN_TAG;
	    break;
	}

	default:
	    TRACE << "Bogus state\n";
	    break;
	}
    }
}

unsigned int BaseParser::Parse(util::StreamPtr, void*, const Data*)
{
    return 0;
}

} // namespace xml

#ifdef TEST

#include "string_stream.h"

class WPLObserver
{
public:
    unsigned int OnMediaSrc(const std::string& s)
    {
	TRACE << "src=" << s << "\n";
	return 0;
    }
};

extern const char MEDIA[] = "media";
extern const char SRC[] = "src";

/* Children=1 */
typedef xml::Parser<xml::Tag<MEDIA,
			     xml::Attribute<SRC, WPLObserver,
					    &WPLObserver::OnMediaSrc>
> > WPLParser;


struct Service
{
    std::string type;
    std::string id;
    std::string control;
    std::string event;
    std::string scpd;
};

class DescObserver
{
public:
    unsigned int OnUDN(const std::string&) { return 0; }
    unsigned int OnFriendlyName(const std::string&) { return 0; }
    unsigned int OnDescription(const std::string&) { return 0; }
    unsigned int OnService(const Service&)
    {
	return 0;
    }
};

extern const char SERVICE[] = "service";
extern const char SERVICETYPE[] = "serviceType";
extern const char SERVICEID[] = "serviceId";
extern const char CONTROLURL[] = "controlURL";
extern const char EVENTSUBURL[] = "eventSubURL";
extern const char SCPDURL[] = "SCPDURL";
extern const char UDN[] = "UDN";
extern const char FRIENDLYNAME[] = "friendlyName";
extern const char PRESENTATIONURL[] = "presentationURL";
extern const char FRINK[] = "frink";
extern const char PTANG[] = "ptang";

/* Children = 6,7 */
typedef xml::Parser<xml::Structure<SERVICE, Service,
				   DescObserver, &DescObserver::OnService,
				   xml::StructureContent<SERVICETYPE, Service,
							 &Service::type>,
				   xml::StructureContent<SERVICEID, Service,
							 &Service::id>,
				   xml::StructureContent<CONTROLURL, Service,
							 &Service::control>,
				   xml::StructureContent<EVENTSUBURL, Service,
							 &Service::event>,
				   xml::StructureContent<SCPDURL, Service,
							 &Service::scpd>,
				   xml::StructureContent<PTANG, Service,
							 &Service::event>,
				   xml::StructureContent<FRINK, Service,
							 &Service::scpd> >,
		    xml::Tag<UDN,
			     xml::Attribute<SRC, DescObserver,
					    &DescObserver::OnUDN> >,
		    xml::Tag<FRIENDLYNAME,
			     xml::Attribute<SRC, DescObserver,
					    &DescObserver::OnFriendlyName> >,
		    xml::Tag<PRESENTATIONURL,
			     xml::Attribute<SRC, DescObserver,
					    &DescObserver::OnDescription> >,
		    xml::Tag<PTANG,
			     xml::Attribute<SRC, DescObserver,
					    &DescObserver::OnFriendlyName> >,
		    xml::Tag<FRINK,
			     xml::Attribute<SRC, DescObserver,
					    &DescObserver::OnDescription> >
		    > DescParser67;

/* Children = 4,5 */
typedef xml::Parser<xml::Structure<SERVICE, Service,
				   DescObserver, &DescObserver::OnService,
				   xml::StructureContent<SERVICETYPE, Service,
							 &Service::type>,
				   xml::StructureContent<SERVICEID, Service,
							 &Service::id>,
				   xml::StructureContent<CONTROLURL, Service,
							 &Service::control>,
				   xml::StructureContent<EVENTSUBURL, Service,
							 &Service::event>,
				   xml::StructureContent<SCPDURL, Service,
							 &Service::scpd> >,
		    xml::Tag<UDN,
			     xml::Attribute<SRC, DescObserver,
					    &DescObserver::OnUDN> >,
		    xml::Tag<FRIENDLYNAME,
			     xml::Attribute<SRC, DescObserver,
					    &DescObserver::OnFriendlyName> >,
		    xml::Tag<PRESENTATIONURL,
			     xml::Attribute<SRC, DescObserver,
					    &DescObserver::OnDescription> >
		    > DescParser;

/* Children = 2,3 */
typedef xml::Parser<xml::Structure<SERVICE, Service,
				   DescObserver, &DescObserver::OnService,
				   xml::StructureContent<CONTROLURL, Service,
							 &Service::control>,
				   xml::StructureContent<EVENTSUBURL, Service,
							 &Service::event>,
				   xml::StructureContent<SCPDURL, Service,
							 &Service::scpd> >,
		    xml::Tag<PRESENTATIONURL,
			     xml::Attribute<SRC, DescObserver,
					    &DescObserver::OnDescription> >
		    > DescParser23;

struct SaxEvent {
    char event;
    const char *arg1;
    const char *arg2;
};

const SaxEvent events1[] = {
    { 'b', "wpl", NULL },
    { 'b', "media", NULL },
    { 'a', "src", "foo" },
    { 'c', "bar", NULL },
    { 'e', "media", NULL },
    { 'e', "wpl", NULL },
    { 'f', NULL, NULL }
};

const SaxEvent events2[] = {
    { 'b', "wpl", NULL },
    { 'b', "foo", NULL },
    { 'b', "bar", NULL },
    { 'e', "bar", NULL },
    { 'e', "foo", NULL },
    { 'e', "wpl", NULL },
    { 'f', NULL, NULL }
};

const SaxEvent events3[] = {
    { 'b', "wpl", NULL },
    { 'b', "foo", NULL },
    { 'a', "name", "wurdle" },
    { 'b', "bar", NULL },
    { 'a', "name", "frink" },
    { 'e', "bar", NULL },
    { 'e', "foo", NULL },
    { 'e', "wpl", NULL },
    { 'f', NULL, NULL }
};

const SaxEvent events4[] = {
    { 'b', "wpl", NULL },
    { 'b', "foo", NULL },
    { 'a', "name", "wurdle" },
    { 'b', "bar", NULL },
    { 'a', "name", "ptang" },
    { 'e', "bar", NULL },
    { 'e', "foo", NULL },
    { 'e', "wpl", NULL },
    { 'f', NULL, NULL }
};

const SaxEvent events5[] = {
    { 'b', "wpl", NULL },
    { 'b', "foo", NULL },
    { 'a', "name", "" },
    { 'a', "wurdle", "" },
    { 'b', "bar", NULL },
    { 'a', "name", "X&Y" },
    { 'e', "bar", NULL },
    { 'e', "foo", NULL },
    { 'e', "wpl", NULL },
    { 'f', NULL, NULL }
};

const SaxEvent events6[] = {
    { 'b', "wpl", NULL },
    { 'b', "foo", NULL },
    { 'a', "wurdle", "" },
    { 'b', "bar", NULL },
    { 'a', "name", "" },
    { 'e', "bar", NULL },
    { 'e', "foo", NULL },
    { 'e', "wpl", NULL },
    { 'f', NULL, NULL }
};

const SaxEvent events7[] = {
    { 'b', "wpl", NULL },
    { 'b', "foo", NULL },
    { 'a', "name", "" },
    { 'a', "wurdle", "" },
    { 'b', "bar", NULL },
    { 'a', "name", "X&Y" },
    { 'a', "frink", "" },
    { 'e', "bar", NULL },
    { 'e', "foo", NULL },
    { 'e', "wpl", NULL },
    { 'f', NULL, NULL }
};
    
const struct {
    const char *xml;
    const SaxEvent *events;
} saxtests[] = {
    { "<wpl><media src=\"foo\">bar</media></wpl>", events1 },
    { "<wpl><media \t  src=\"foo\"  >bar</media></wpl>", events1 },
    { "<wpl><foo><bar/></foo></wpl>", events2 },
    { "<wpl><foo name=wurdle><bar name=frink/></foo></wpl>", events3 },
    { "<wpl><foo name=wurdle><bar name=\"ptang\"/></foo></wpl>", events4 },
    { "<wpl><foo name = wurdle><bar name = \"ptang\"/></foo></wpl>", events4 },
    { "<wpl><foo name wurdle><bar name=\"X&amp;Y\"/></foo></wpl>", events5 },
    { "<wpl><foo wurdle><bar name/></foo></wpl>", events6 },
    { "<wpl><foo \t wurdle \t \t ><bar \t name \t /></foo \t ></wpl>", events6 },
    { "<wpl><foo name wurdle><bar name=\"X&amp;Y\" frink/></foo></wpl>", events7 },
};

enum { TESTS = sizeof(saxtests)/sizeof(saxtests[0]) };

class SaxTestObserver: public xml::SaxParserObserver
{
    const SaxEvent *m_events;
    unsigned int m_count;
    bool m_in_content;
    std::string m_content;

    void CheckContent()
    {
	if (m_in_content)
	{
	    assert(m_events[m_count].event == 'c');
	    assert(!strcmp(m_content.c_str(), m_events[m_count].arg1));
	    ++m_count;
	    m_in_content = false;
	    m_content.clear();
	}
    }

public:
    SaxTestObserver(const SaxEvent *e) : m_events(e), m_count(0), m_in_content(false) {}
    ~SaxTestObserver()
    {
	CheckContent();
	assert(m_events[m_count].event == 'f');
//	TRACE << "--\n";
    }

    // Being a SaxParserObserver
    unsigned int OnBegin(const char *tag)
    {
	CheckContent();
//	TRACE << "OnBegin(" << tag << ")\n";
	assert(m_events[m_count].event == 'b');
	assert(!strcmp(tag, m_events[m_count].arg1));
	++m_count;
	return 0;
    }
    unsigned int OnEnd(const char *tag)
    {
	CheckContent();
//	TRACE << "OnEnd(" << tag << ")\n";
	assert(m_events[m_count].event == 'e');
	assert(!strcmp(tag, m_events[m_count].arg1));
	++m_count;
	return 0;
    }
    unsigned int OnContent(const char *tag)
    {
//	TRACE << "OnContent(" << tag << ")\n";
	m_in_content = true;
	m_content += tag;
	return 0;
    }
    unsigned int OnAttribute(const char *tag, const char *value)
    {
	assert(!m_in_content);
//	TRACE << "OnAttribute(" << tag << "," << value << ")\n";
	assert(m_events[m_count].event == 'a');
	assert(!strcmp(tag, m_events[m_count].arg1));
	assert(!strcmp(value, m_events[m_count].arg2));
	++m_count;
	return 0;
    }
};

int main()
{
    WPLParser parser;

    util::StringStreamPtr ss = util::StringStream::Create();

    ss->str() += "<wpl><media src=\"foo\">bar</media></wpl>";

//    util::StreamPtr ss = 42;

    WPLObserver obs;
    parser.Parse(ss, &obs);

    DescParser dp;
    DescParser23 dp23;
    DescParser67 dp67;
    DescObserver dobs;
    dp.Parse(ss, &dobs);
    dp23.Parse(ss, &dobs);
    dp67.Parse(ss, &dobs);

    for (unsigned int i=0; i<TESTS; ++i)
    {
	util::StringStreamPtr ssp = util::StringStream::Create();
	ssp->str() = saxtests[i].xml;
	size_t len = ssp->str().length();

	for (size_t j=len; j; --j)
	{
//	    TRACE << "j=" << j << "\n";
	    util::SeekableStreamPtr dribble
		= util::DribbleStream::Create(ssp, j);

	    SaxTestObserver sto(saxtests[i].events);
	    xml::SaxParser saxp(&sto);
	    unsigned int rc = saxp.Parse(dribble);
	    assert(rc == 0);
	}
    }

    return 0;
}

#endif
