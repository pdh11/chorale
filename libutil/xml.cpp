#include "xml.h"
#include "libutil/trace.h"

namespace xml {

unsigned int SaxParser::Parse(util::StreamPtr s)
{
    enum { BUFSIZE = 1024 };
    char buffer[BUFSIZE+1];
    unsigned int bufused = 0;
    bool eof = false;
    unsigned int rc = 0;
    
    enum { CONTENT, TAG, IN_TAG, SEEK_GT } state = CONTENT;

    for (;;)
    {
	if (bufused < BUFSIZE && !eof)
	{
	    size_t nread;
	    rc = s->Read(buffer+bufused, BUFSIZE-bufused, &nread);
	    if (nread == 0)
		eof = true;
	    bufused += nread;
	}

	if (bufused == 0 && eof)
	    return 0; // Done

	buffer[bufused] = '\0';

	TRACE << "state " << (int)state << " buffer " << bufused << " "
	      << buffer << "\n";

	switch (state)
	{
	case CONTENT:
	{
	    char *lt = strchr(buffer, '<');
	    if (lt == NULL)
	    {
		// xmlunescape
		m_observer->OnContent(buffer);
		bufused = 0;
	    }
	    else
	    {
		*lt = '\0';
		if (lt > buffer)
		{
		    // xmlunescape
		    m_observer->OnContent(buffer);
		}
		state = TAG;
		unsigned int usedup = lt+1 - buffer;
		TRACE << "used up " << usedup << "\n";
		memmove(buffer, lt+1, bufused-usedup);
		bufused -= usedup;
	    }
	    break;
	}
	case TAG:
	{
	    size_t taglen = strcspn(buffer, " \t\r\n>");
	    if (taglen == bufused && !eof && bufused < BUFSIZE)
		continue;
	    char c = buffer[taglen];
	    buffer[taglen] = '\0';
	    if (*buffer == '/')
	    {
		m_observer->OnEnd(buffer+1);
		if (c == '>')
		    state = CONTENT;
		else
		    state = SEEK_GT;
	    }
	    else
	    {
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
	    bufused -= (taglen+1);
	    break;
	}
	
	case SEEK_GT:
	{
	    char *gt = strchr(buffer, '>');
	    if (gt == NULL && !eof && bufused < BUFSIZE)
		continue;
	    unsigned int usedup = gt+1 - buffer;
	    memmove(buffer, gt+1, bufused-usedup);
	    bufused -= usedup;
	    state = CONTENT;
	    break;
	}

	case IN_TAG:
	    state = SEEK_GT;
	    break;

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
    { 'c', "bar", NULL },
    { 'e', "media", NULL },
    { 'e', "wpl", NULL },
    { 'f', NULL, NULL }
};
    
const struct {
    const char *xml;
    const SaxEvent *events;
} saxtests[] = {
    { "<wpl><media src=\"foo\">bar</media></wpl>", events1 }
};

enum { TESTS = sizeof(saxtests)/sizeof(saxtests[0]) };

class SaxTestObserver: public xml::SaxParserObserver
{
    const SaxEvent *m_events;
    unsigned int m_count;
public:
    SaxTestObserver(const SaxEvent *e) : m_events(e), m_count(0) {}
    ~SaxTestObserver()
    {
	assert(m_events[m_count].event == 'f');
    }

    // Being a SaxParserObserver
    unsigned int OnBegin(const char *tag)
    {
	TRACE << "OnBegin(" << tag << ")\n";
	assert(m_events[m_count].event == 'b');
	assert(!strcmp(tag, m_events[m_count].arg1));
	++m_count;
	return 0;
    }
    unsigned int OnEnd(const char *tag)
    {
	TRACE << "OnEnd(" << tag << ")\n";
	assert(m_events[m_count].event == 'e');
	assert(!strcmp(tag, m_events[m_count].arg1));
	++m_count;
	return 0;
    }
    unsigned int OnContent(const char *tag)
    {
	TRACE << "OnContent(" << tag << ")\n";
	assert(m_events[m_count].event == 'c');
	assert(!strcmp(tag, m_events[m_count].arg1));
	++m_count;
	return 0;
    }
    unsigned int OnAttribute(const char *tag, const char *value)
    {
	TRACE << "OnAttribute(" << tag << "," << value << ")\n";
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
	SaxTestObserver sto(saxtests[i].events);
	xml::SaxParser saxp(&sto);
	unsigned int rc = saxp.Parse(ssp);
	assert(rc == 0);
    }

    return 0;
}

#endif
