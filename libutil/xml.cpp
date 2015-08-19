#include "xml.h"
#include "trace.h"
#include "errors.h"
#include "stream.h"
#include "xmlescape.h"
#include <string.h>
#include <list>

namespace xml {

SaxParser::SaxParser(SaxParserObserver *observer)
    : m_observer(observer),
      m_buffered(0),
      m_eof(false),
      m_state(CONTENT)
{
}

unsigned int SaxParser::Parse(util::Stream *s)
{
    unsigned int rc = 0;

    for (;;)
    {
	if (m_buffered < BUFSIZE && !m_eof)
	{
	    size_t nread = 0;
	    rc = s->Read(m_buffer+m_buffered, BUFSIZE-m_buffered, &nread);
	    if (rc)
	    {
		TRACE << "Read gave error " << rc << "\n";
		return rc;
	    }
//	    TRACE << "Read got me " << nread << " bytes\n";
	    if (nread == 0)
		m_eof = true;
	    m_buffered += (unsigned)nread;
	}

	if (m_buffered == 0 && m_eof)
	    return 0; // Done

	m_buffer[m_buffered] = '\0';

	Parse();
    }
}

unsigned int SaxParser::WriteAll(const void *buffer, size_t len)
{
    while (len)
    {
	if (m_buffered < BUFSIZE)
	{
	    size_t lump = std::min(len, BUFSIZE - m_buffered);
	    memcpy(m_buffer + m_buffered, (const char*)buffer, lump);
	    m_buffered += lump;
	    len -= lump;
	    buffer = (const void*)((const char*)buffer + lump);
	}

	m_buffer[m_buffered] = '\0';
	Parse();
    }

    return 0;
}

void SaxParser::Parse()
{
    while (m_buffered)
    {
//	TRACE << "st " << (int)m_state
//	      << " eof " << m_eof
//	      << " buf " << m_buffered << " " << m_buffer << "\n";

	switch (m_state)
	{
	case CONTENT:
	{
	    char *lt = strchr(m_buffer, '<');
	    if (lt == NULL)
	    {
		// xmlunescape
		char *amp = strrchr(m_buffer, '&');
		char *semi = strrchr(m_buffer, ';');
		if (amp && (!semi || semi<amp))
		{
		    // Take care not to split up an entity &qu .... ot;
		    *amp = '\0';
		    std::string unesc = util::XmlUnEscape(m_buffer);
		    m_observer->OnContent(unesc.c_str());
		    *amp = '&';
		    unsigned int usedup = (unsigned)(amp-m_buffer);
		    memmove(m_buffer, amp, m_buffered - usedup + 1);
		    m_buffered -= usedup;
		    if (!m_eof)
			return; // Can't do anything until we see rest of it
		}
		else
		{
		    std::string unesc = util::XmlUnEscape(m_buffer);
		    m_observer->OnContent(unesc.c_str());
		    m_buffered = 0;
		}
	    }
	    else
	    {
		*lt = '\0';
		if (lt > m_buffer)
		{
		    // xmlunescape
		    std::string unesc = util::XmlUnEscape(m_buffer);
		    m_observer->OnContent(unesc.c_str());
		}
		m_state = TAG;
		unsigned int usedup = (unsigned)(lt+1 - m_buffer);
//		TRACE << "used up " << usedup << "\n";
		memmove(m_buffer, lt+1, m_buffered-usedup+1);
		m_buffered -= usedup;
	    }
	    break;
	}

	case CDATA:
	{
	    char *ket = strchr(m_buffer, ']');
	    if (ket == NULL)
	    {
		// No xmlunescape -- this is sparta^W cdata
		m_observer->OnContent(m_buffer);
		m_buffered = 0;
	    }
	    else
	    {
		if (ket > m_buffer)
		{
		    *ket = '\0';
		    m_observer->OnContent(m_buffer);
		    *ket = ']';
		    size_t usedup = ket-m_buffer;
		    memmove(m_buffer, ket, m_buffered - usedup + 1);
		    m_buffered -= usedup;
		    break;
		}
		if (m_buffered < 3)
		{
		    return;
		}
		if (m_buffer[1] == ']' && m_buffer[2] == '>')
		{
		    // "]]>" end-of-cdata sequence
		    memmove(m_buffer, m_buffer+3, m_buffered-3 + 1);
		    m_buffered -= 3;
		    m_state = CONTENT;
		    break;
		}
		// Not end-of-cdata sequence -- pass it on
		m_observer->OnContent("]");
		memmove(m_buffer, m_buffer+1, m_buffered-1 + 1);
		--m_buffered;
	    }
	    break;
	}

	case TAG:
	{
	    bool end = false;
	    size_t skip = 0;

	    const size_t CDATASTART = strlen("![CDATA[");

	    size_t tocompare = std::min(m_buffered, CDATASTART);
	    if (!memcmp(m_buffer, "![CDATA[", tocompare))
	    {
		if (m_buffered < CDATASTART)
		    return;
		m_state = CDATA;
		memmove(m_buffer, m_buffer+CDATASTART, m_buffered-CDATASTART+1);
		m_buffered -= CDATASTART;
		break;
	    }
	    
	    if (*m_buffer == '/')
	    {
		end = true;
		skip = 1;
	    }
	    size_t taglen = strcspn(m_buffer + skip, " \t\r\n/>") + skip;
	    if (taglen == m_buffered && !m_eof && m_buffered < BUFSIZE)
		return;
	    char c = m_buffer[taglen];
	    m_buffer[taglen] = '\0';
	    if (end)
	    {
		m_observer->OnEnd(m_buffer+1);
		if (c == '>')
		    m_state = CONTENT;
		else
		    m_state = SEEK_GT;
	    }
	    else
	    {
		m_tagname = m_buffer;
		m_observer->OnBegin(m_buffer);
		if (c == '/')
		{
		    m_observer->OnEnd(m_buffer);
		    m_state = SEEK_GT;
		}
		else if (c == '>')
		    m_state = CONTENT;
		else
		    m_state = IN_TAG;
	    }
	    memmove(m_buffer, m_buffer+taglen+1, m_buffered-taglen-1+1);
	    m_buffered -= (unsigned)(taglen+1);
	    break;
	}
	
	case SEEK_GT:
	{
	    char *gt = strchr(m_buffer, '>');
	    if (gt == NULL && !m_eof && m_buffered < BUFSIZE)
		return;
	    unsigned int usedup = (unsigned)(gt+1 - m_buffer);
	    memmove(m_buffer, gt+1, m_buffered-usedup + 1);
	    m_buffered -= usedup;
	    m_state = CONTENT;
	    break;
	}

	// <tag  attr=thing>
	//     ^
	case IN_TAG:
	{
	    size_t skip = strspn(m_buffer, " \t\n\r");
	    if (skip == m_buffered)
	    {
		m_buffered = 0;
		break;
	    }
	    char c = m_buffer[skip];
	    if (c == '>')
		m_state = SEEK_GT;
	    else if (c == '/')
	    {
		m_observer->OnEnd(m_tagname.c_str());
		++skip;
	    }
	    else
		m_state = IN_ATTR;
	    memmove(m_buffer, m_buffer+skip, m_buffered - skip + 1);
	    m_buffered -= skip;
	    break;
	}

	// <tag  attr=thing>
	//       ^
	case IN_ATTR:
	{
	    size_t attrlen = strcspn(m_buffer, " \t\n\r=/>");
	    if (attrlen == m_buffered && !m_eof && m_buffered < BUFSIZE)
		return;
	    char c = m_buffer[attrlen];
	    m_buffer[attrlen] = '\0';
	    m_attrname = m_buffer;
	    if (c == '=')
		m_state = ATTR_SEEKVALUE;
	    else if (c == '>')
	    {
		m_observer->OnAttribute(m_buffer, "");
		m_state = CONTENT;
	    }
	    else if (c == '/')
	    {
		m_observer->OnAttribute(m_buffer, "");
		m_observer->OnEnd(m_tagname.c_str());
		m_state = SEEK_GT;
	    }
	    else
		m_state = ATTR_SEEKEQ;

	    unsigned int usedup = (unsigned)(attrlen+1);
	    memmove(m_buffer, m_buffer+usedup, m_buffered-usedup + 1);
	    m_buffered -= usedup;
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
	    size_t skip = strspn(m_buffer, " \t\n\r");
	    if (skip == m_buffered)
	    {
		m_buffered = 0;
		break;
	    }
	    char c = m_buffer[skip];
	    if (c == '=')
	    {
		m_state = ATTR_SEEKVALUE;
		++skip;
	    }
	    else
	    {
		m_observer->OnAttribute(m_attrname.c_str(), "");
		m_state = IN_TAG;
	    }

	    memmove(m_buffer, m_buffer+skip, m_buffered-skip + 1);
	    m_buffered -= skip;
	    break;
	}

	// <tag attr = "thing"/>
	//            ^
	case ATTR_SEEKVALUE:
	{
	    size_t skip = strspn(m_buffer, " \t\n\r");
	    if (skip == m_buffered)
	    {
		m_buffered = 0;
		break;
	    }
	    char c = m_buffer[skip];
	    if (c == '\"')
	    {
		++skip;
		m_state = IN_QUOTEDVALUE;
	    }
	    else
		m_state = IN_VALUE;

	    memmove(m_buffer, m_buffer+skip, m_buffered - skip + 1);
	    m_buffered -= skip;
	    break;
	}

	// <tag attr = "thing"/>
	//              ^
	case IN_QUOTEDVALUE:
	{
	    char *quote = strchr(m_buffer, '"');
	    if (!quote && !m_eof && m_buffered < BUFSIZE)
		return;
	    *quote = '\0';
	    
	    std::string value = util::XmlUnEscape(m_buffer);
	    m_observer->OnAttribute(m_attrname.c_str(), value.c_str());
	    
	    m_state = IN_TAG;
	    unsigned int usedup = (unsigned)(quote+1 - m_buffer);
	    memmove(m_buffer, quote+1, m_buffered - usedup + 1);
	    m_buffered -= usedup;
	    break;
	}

	// <tag attr = thing/>
	//             ^
	case IN_VALUE:
	{
	    size_t valuelen = strcspn(m_buffer, " \t\r\n/>");
	    if (valuelen == m_buffered && !m_eof && m_buffered < BUFSIZE)
		return;

	    char c = m_buffer[valuelen];
	    m_buffer[valuelen] = '\0';
	    m_observer->OnAttribute(m_attrname.c_str(), 
				    util::XmlUnEscape(m_buffer).c_str());
	    m_buffer[valuelen] = c;
	    memmove(m_buffer, m_buffer+valuelen, m_buffered - valuelen + 1);
	    m_buffered -= valuelen;
	    m_state = IN_TAG;
	    break;
	}

	default:
	    TRACE << "Bogus state " << (int)m_state << "\n";
	    return;
	}
    }
}


        /* Internals of table-driven parser */


namespace details {

namespace {

class TableDrivenParser: public SaxParserObserver
{
    void *m_target;
    const details::Data *m_current;
    std::list<const details::Data*> m_table_stack;
    std::list<void*> m_target_stack;
    unsigned int m_ignore;
    std::string m_content;

public:
    TableDrivenParser(void *target, const details::Data *table)
	: m_target(target),
	  m_current(table),
	  m_ignore(0)
    {
    }

    ~TableDrivenParser()
    {
    }

    unsigned int OnBegin(const char *tag) override
    {
	m_content.clear();

	if (tag[0] == '?')
	    return 0;

	if (m_current && !m_ignore)
	{
	    for (unsigned int i=0; i < m_current->n; ++i)
	    {
		if (!strcasecmp(tag, m_current->children[i]->name)
		    && !m_current->children[i]->IsAttribute())
		{
		    m_table_stack.push_back(m_current);
		    m_target_stack.push_back(m_target);
		    m_current = m_current->children[i];
		    if (m_current->sbegin)
			m_target = (*m_current->sbegin)(m_target);
		    return 0;
		}
	    }
	}

	// Either it's not in our list, or we're at a leaf. Either way,
	// ignore it and all its children
//	TRACE << "Ignoring start " << tag << "\n";
	++m_ignore;
	return 0;
    }

    unsigned int OnAttribute(const char *name, const char *value) override
    {
	if (m_current && !m_ignore)
	{
	    for (unsigned int i=0; i < m_current->n; ++i)
	    {
		if (!strcasecmp(name, m_current->children[i]->name)
		    && m_current->children[i]->IsAttribute())
		{
		    return (*m_current->children[i]->text)(m_target, value);
		}
	    }
	}
	return 0;
    }

    unsigned int OnContent(const char *content) override
    {
	if (m_current)
	    m_content += content;
	return 0;
    }

    unsigned int OnEnd(const char *) override
    {
	if (m_ignore)
	{
	    --m_ignore;
	    return 0;
	}

	if (m_table_stack.empty())
	    return EINVAL; // Ill-formed XML

	if (m_current)
	{
	    if (m_current->text)
		(*m_current->text)(m_target, m_content);
	}

	m_target = m_target_stack.back();
	m_target_stack.pop_back();

	m_current = m_table_stack.back();
	m_table_stack.pop_back();
	return 0;
    }
};

} // anon namespace

unsigned int Parse(util::Stream *sp, void *target,
		   const details::Data *table)
{
    TableDrivenParser tdp(target, table);
    SaxParser saxp(&tdp);
    return saxp.Parse(sp);
}

template<>
const Data *const ArrayHelper<>::data[] = {NULL};

} // namespace details

/** @page xml XML parsing in C++

The UPnP A/V standards use XML heavily; the Chorale implementation
needs parsers for several, mostly small, XML schemas.

Initially these all used a SAX-style parsing scheme implemented in
xml::SaxParser, requiring a (usually quite wordy) custom parser
observer for each use. But the problem seemed to be calling out for a
DSEL ("domain-specific embedded language"); that is, for some
template-fu that made declaring individual little parsers easy.
 
The solution adopted in Chorale is to use table-based parsers, and to
simplify declaring such table-based parsers by generating them as
static data of classes built from nested templates. For instance,
parsing the "src" attribute out of XML that looks like this:

@verbatim
<wpl><media src="/home/bob/music/Abba/Fernando.flac">bar</media></wpl>
@endverbatim

can be done as follows:

@code
class WPLReader
{
public:
    unsigned int Parse(Stream *stm);

    unsigned int OnMediaSrc(const std::string& s)
    {
	TRACE << "src=" << s << "\n";
	return 0;
    }
};

constexpr const char WPL[] = "wpl";
constexpr const char MEDIA[] = "media";
constexpr const char SRC[] = "src";

typedef xml::Parser<xml::Tag<WPL,
			     xml::Tag<MEDIA,
				      xml::Attribute<SRC, WPLReader,
						     &WPLReader::OnMediaSrc>
> > > WPLParser;

unsigned int WPLReader::Parse(Stream *stm)
{
    WPLParser parser;
    return parser.Parse(stm, this);
}

@endcode

...where the nesting of the "xml::" templates, mirrors the nesting of
the tags in the XML we're trying to parse. The typedef declares a
class type WPLParser, with the single method Parse, which takes a
Stream (the usual abstraction for a byte stream in Chorale) and a
pointer to the <i>target object</i>: the object which receives
callbacks and data from the parser. The <tt>unsigned int</tt> returned
from these functions is the usual Chorale way of indicating an error:
0 means successful completion, values from \c <errno.h> mean otherwise.

(As is often the case with DSELs in C++, some technicalities leak out
into the user-experience: in this case, string literals are not
allowed as template parameters, and nor are objects with internal
linkage, so we must declare \c constexpr objects corresponding to each of
the strings we want to look for. Also, in the invocation of
xml::Attribute, C++ isn't able to deduce the type "WPLReader" from the
method pointer "&WPLReader::OnMediaSrc", or vice versa, so "WPLReader"
must be specified twice.)

The relevant templates are:

@li xml::Parser should be the outermost template. It does no processing itself,
    but ensures that templates nested inside it, only match top-level elements.

@li xml::Tag does no processing, but ensures that templates nested inside it,
    only match tags nested inside its tag. For instance, with the above parser,
    only \c media tags that are direct children of a \c wpl tag are matched.

@li xml::Attribute matches attributes of the current tag, and reports them
    to a callback function, specified as a method on the current target object.

@li xml::TagCallback ensures nesting like Tag does, but also collects up the
    textual contents of the tag (like the "bar" in the above XML) and reports
    them to a callback function, again specified as a method on the current
    target object.

@li xml::TagMember again ensures nesting, and again collects contents, but
    instead of calling a method pointer, it stores the string directly into
    a string member of the current target object.

@li xml::Structure ensures nesting like xml::Tag does, and also specifies a new
    target object, which should be a member (sub-object) of its parent's
    target object.

@li xml::List is the most powerful template. It ensures nesting, and specifies
    a new target object which is the element type of a std::list member of its
    parent's target object.

Here is a more complex example demonstrating the use of xml::Structure
and xml::List. It's based on the parser for UPnP device description documents
in libupnp/description.cpp.

@code
struct Service
{
    std::string type;
    std::string id;
    std::string control;
    std::string event;
    std::string scpd;
};

struct Device
{
    std::string type;
    std::string friendly_name;
    std::string udn;
    std::string presentation_url;
    std::list<Service> services;
};

struct Description
{
    std::string url_base;
    Device root_device;
};

typedef xml::Parser<
    xml::Tag<ROOT,
	     xml::TagMember<URLBASE, Description,
			    &Description::url_base>,
	     xml::Structure<DEVICE, Device,
			    Description, &Description::root_device,
			    xml::TagMember<DEVICETYPE, Device,
					   &Device::type>,
			    xml::TagMember<FRIENDLYNAME, Device,
					   &Device::friendly_name>,
			    xml::TagMember<UDN, Device,
					   &Device::udn>,
			    xml::TagMember<PRESENTATIONURL, Device,
					   &Device::presentation_url>,
			    xml::Tag<SERVICELIST,
				     xml::List<SERVICE, Service,
					       Device, &Device::services,
					       xml::TagMember<SERVICETYPE, Service,
							      &Service::type>,
					       xml::TagMember<SERVICEID, Service,
							      &Service::id>,
					       xml::TagMember<CONTROLURL, Service,
							      &Service::control>,
					       xml::TagMember<EVENTSUBURL, Service,
							      &Service::event>,
					       xml::TagMember<SCPDURL, Service,
							      &Service::scpd>
> > > > > DescriptionParser;
@endcode

As always, the nested form of the parser corresponds to the nested form of the XML:

@verbatim
<root xmlns="urn:schemas-upnp-org:device-1-0">
  <URLBase>http://192.168.168.1:49152/</URLBase>
  <device>
    <deviceType>urn:schemas-upnp-org:device:MediaServer:1</deviceType>
    <friendlyName>/media/mp3audio on jalfrezi</friendlyName>
    <UDN>uuid:726f6863-2065-6c61-00de-df6268fff5a0</UDN>
    <presentationURL>http://192.168.168.1:12078/</presentationURL>
    <serviceList>
      <service>
        <serviceType>urn:schemas-upnp-org:service:ContentDirectory:1</serviceType>
        <serviceId>urn:upnp-org:serviceId:ContentDirectory</serviceId>
        <SCPDURL>http://192.168.168.1:12078/upnp/ContentDirectory.xml</SCPDURL>
        <controlURL>/upnpcontrol0</controlURL>
        <eventSubURL>/upnpevent0</eventSubURL>
      </service>
      <service>
        <serviceType>urn:schemas-upnp-org:service:HornSwoggler:1</serviceType>
        <serviceId>urn:upnp-org:serviceId:HornSwoggler</serviceId>
        <SCPDURL>http://192.168.168.1:12078/upnp/HornSwoggler.xml</SCPDURL>
        <controlURL>/upnpcontrol1</controlURL>
        <eventSubURL>/upnpevent1</eventSubURL>
      </service>
    </serviceList>
  </device>
</root>
@endverbatim

Calling DescriptionParser::Parse, passing the above XML and a Description
object as the target, would end up filling-in the \c url_base member, the
\c root_device structure, and a two-element list in \c root_device.services.

In the above example, the <i>order</i> of the elements in the XML
happens to correspond to the ordering in the parser declaration,
too. In general, this won't be the case: xml::List preserves ordering of its child elements, but
the other templates don't. Parsers built using these classes can't be used where preserving the order of heterogenous tags is required: for instance, in XHTML.

In each case, the target object of all sibling templates must be the
same. All child templates of an xml::Structure must use the structure
type as the type of their target object; all child templates of an
xml::List must use the list-element type; and all other child
templates must use their parent template's type. (The root template,
xml::Parser, must use the type of the target object passed to
xml::Parser::Parse.) These requirements are enforced with compile-time
    assertions -- although, as seems unavoidable with C++ DSELs, it's hard to see the wood for the trees in the resulting error messages
if you get it wrong. Errors involving "AssertSame" or
"AssertCorrectTargetType" usually mean that target types have been
muddled somewhere.


@section impl Implementation

The generated XML parsers are extremely compact, both in static and
dynamic memory usage. The actual parsing is done by the xml::SaxParser
class, via an adaptor (xml::details::TableDrivenParser) which follows tables
(xml::details::Data) telling it what to do on encountering the various tags.

Each template invocation corresponds to one table entry, which on
	   i686-linux is 20 bytes (plus the \c char* storage for the tag name). Tables are const, and so end up in the \c rodata segment. The size of an xml::Parser is also very small
(1 byte), as all the other templates only have their static data
referenced -- they aren't actually instantiated anywhere.

    This diagram depicts some of the tables created for the device-description
    example parser above:

@dot
digraph parser
{
node [shape=record, fontname=Helvetica, fontsize=10];

parser [label="{NULL | NULL | 1 | <p0>}"];

root [label="{\"root\" | NULL | 2 | <p1>}"];

parser:p0 -> root:nw;

urlbase [label="{ \"URLBase\" | &Description::url_base | 0 | NULL }|{ \"Device\" | &Description::root_device | 5 | <p3> }"];

root:p1 -> urlbase:nw;

device [label="{ \"deviceType\" | &Device::type | 0 | NULL}|{ \"friendlyName\" | &Device::friendly_name | 0 | NULL }|{ \"UDN\" | &Device::udn | 0 | NULL}|{ \"presentationURL\" | &Device::presentation_url | 0 | NULL }|{ \"serviceList\" | NULL | 1 | <p2>}"];

urlbase:p3       -> device:nw;

}
@enddot

The diagram is a slight simplification: to achieve type erasure, the tables
don't actually contain the pointers-to-members shown above, but instead
pointers to functions (such as TagMember::OnText) that do the upcast
from \c void* and reference the correct member.

Now that the code uses C++14 and its variadic template support, the
previous restriction on table sizes (eight) has been lifted.

 */

} // namespace xml

#ifdef TEST

# include "string_stream.h"
# include "stream_test.h"

class WPLObserver
{
public:
    unsigned int OnMediaSrc(const std::string&)
    {
//	TRACE << "src=" << s << "\n";
	return 0;
    }
};

constexpr const char WPL[] = "wpl";
constexpr const char MEDIA[] = "media";
constexpr const char SRC[] = "src";

typedef xml::Parser<xml::Tag<WPL,
			     xml::Tag<MEDIA,
				      xml::Attribute<SRC, WPLObserver,
						     &WPLObserver::OnMediaSrc>
> > > WPLParser;


struct Service
{
    std::string type;
    std::string id;
    std::string control;
    std::string event;
    std::string scpd;
    std::string ptang;
    std::string frink;
};

struct Device
{
    std::string type;
    std::string friendly_name;
    std::string udn;
    std::string presentation_url;
    std::list<Service> services;
    std::list<Device> devices;
};

struct DescObserver
{
    std::string url_base;
    Device root_device;
};

constexpr const char SERVICELIST[] = "serviceList";
constexpr const char SERVICE[] = "service";
constexpr const char SERVICE_TYPE[] = "serviceType"; // clashes with winsock2.h
constexpr const char SERVICEID[] = "serviceId";
constexpr const char CONTROLURL[] = "controlURL";
constexpr const char EVENTSUBURL[] = "eventSubURL";
constexpr const char SCPDURL[] = "SCPDURL";
constexpr const char UDN[] = "UDN";
constexpr const char FRIENDLYNAME[] = "friendlyName";
constexpr const char PRESENTATIONURL[] = "presentationURL";
constexpr const char FRINK[] = "frink";
constexpr const char PTANG[] = "ptang";

constexpr const char ROOT[] = "root";
constexpr const char URLBASE[] = "URLBase";
constexpr const char DEVICE[] = "device";
constexpr const char DEVICETYPE[] = "deviceType";

typedef xml::Parser<
    xml::Tag<ROOT,
	     xml::TagMember<URLBASE, DescObserver,
			    &DescObserver::url_base>,
	     xml::Structure<DEVICE, Device,
			    DescObserver, &DescObserver::root_device,
			    xml::TagMember<DEVICETYPE, Device,
					   &Device::type>,
			    xml::TagMember<FRIENDLYNAME, Device,
					   &Device::friendly_name>,
			    xml::TagMember<UDN, Device,
					   &Device::udn>,
			    xml::TagMember<PRESENTATIONURL, Device,
					   &Device::presentation_url>,
			    xml::Tag<SERVICELIST,
				     xml::List<SERVICE, Service,
					       Device, &Device::services,
					       xml::TagMember<SERVICE_TYPE, Service,
							      &Service::type>,
					       xml::TagMember<SERVICEID, Service,
							      &Service::id>,
					       xml::TagMember<CONTROLURL, Service,
							      &Service::control>,
					       xml::TagMember<EVENTSUBURL, Service,
							      &Service::event>,
					       xml::TagMember<SCPDURL, Service,
							      &Service::scpd>
> > > > > DescriptionParser;

void TestTableDriven()
{
    WPLParser parser;

    util::StringStream ss("<wpl><media src=\"foo\">bar</media></wpl>");

    WPLObserver obs;
    parser.Parse(&ss, &obs);

    ss.str() = 
"<?xml version=\"1.0\"?>"
"<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
"<specVersion>"
"<major>1</major>"
"<minor>0</minor>"
"</specVersion>"
"<device>"
"<deviceType>urn:schemas-upnp-org:device:MediaServer:1</deviceType>"
"<friendlyName>/media/mp3audio on jalfrezi</friendlyName>"
"<manufacturer>Chorale contributors</manufacturer>"
"<manufacturerURL>http://chorale.sf.net</manufacturerURL>"
"<modelDescription>0.12</modelDescription>"
"<modelName>chorale</modelName>"
"<modelNumber>0.12</modelNumber>"
"<UDN>uuid:726f6863-2065-6c61-00de-df6268fff5a0</UDN>"
"<presentationURL>http://192.168.168.1:12078/</presentationURL>"
"<iconList>"
"<icon>"
"<mimetype>image/png</mimetype>"
"<width>32</width>"
"<height>32</height>"
"<depth>24</depth>"
"<url>http://192.168.168.1:12078/layout/icon.png</url>"
"</icon>"
"<icon>"
"<mimetype>image/vnd.microsoft.icon</mimetype>"
"<width>32</width>"
"<height>32</height>"
"<depth>24</depth>"
"<url>http://192.168.168.1:12078/layout/icon.ico</url>"
"</icon>"
"<icon>"
"<mimetype>image/x-icon</mimetype>"
"<width>32</width>"
"<height>32</height>"
"<depth>24</depth>"
"<url>http://192.168.168.1:12078/layout/icon.ico</url>"
"</icon>"
"</iconList>"
"<serviceList>"
"<service>"
"<serviceType>urn:schemas-upnp-org:service:ContentDirectory:1</serviceType>"
"<serviceId>urn:upnp-org:serviceId:ContentDirectory</serviceId>"
"<SCPDURL>http://192.168.168.1:12078//upnp/ContentDirectory.xml</SCPDURL>"
"<controlURL>/upnpcontrol0</controlURL>"
"<eventSubURL>/upnpevent0</eventSubURL>"
"</service>"
"<service>"
"<serviceType>urn:schemas-upnp-org:service:HornSwoggler:1</serviceType>"
"<serviceId>urn:upnp-org:serviceId:HornSwoggler</serviceId>"
"<SCPDURL>http://192.168.168.1:12078//upnp/HornSwoggler.xml</SCPDURL>"
"<controlURL>/upnpcontrol1</controlURL>"
"<eventSubURL>/upnpevent1</eventSubURL>"
"</service>"
"</serviceList>"
"<deviceList>"
"<device>"
"<deviceType>urn:chorale-sf-net:dev:OpticalDrive:1</deviceType>"
"<friendlyName>/dev/hdc on jalfrezi</friendlyName>"
"<manufacturer>Chorale contributors</manufacturer>"
"<manufacturerURL>http://chorale.sf.net</manufacturerURL>"
"<modelDescription>0.12</modelDescription>"
"<modelName>chorale</modelName>"
"<modelNumber>0.12</modelNumber>"
"<UDN>uuid:726f6863-2065-6c61-00de-df62f9029835</UDN>"
"<presentationURL>http://192.168.168.1:12078/</presentationURL>"
"<iconList>"
"<icon>"
"<mimetype>image/png</mimetype>"
"<width>32</width>"
"<height>32</height>"
"<depth>24</depth>"
"<url>http://192.168.168.1:12078/layout/icon.png</url>"
"</icon>"
"<icon>"
"<mimetype>image/vnd.microsoft.icon</mimetype>"
"<width>32</width>"
"<height>32</height>"
"<depth>24</depth>"
"<url>http://192.168.168.1:12078/layout/icon.ico</url>"
"</icon>"
"<icon>"
"<mimetype>image/x-icon</mimetype>"
"<width>32</width>"
"<height>32</height>"
"<depth>24</depth>"
"<url>http://192.168.168.1:12078/layout/icon.ico</url>"
"</icon>"
"</iconList>"
"<serviceList>"
"<service>"
"<serviceType>urn:chorale-sf-net:service:OpticalDrive:1</serviceType>"
"<serviceId>urn:chorale-sf-net:serviceId:OpticalDrive</serviceId>"
"<SCPDURL>http://192.168.168.1:12078//upnp/OpticalDrive.xml</SCPDURL>"
"<controlURL>/upnpcontrol2</controlURL>"
"<eventSubURL>/upnpevent2</eventSubURL>"
"</service>"
"</serviceList>"
"</device>"
"</deviceList>"
"</device>"
"<URLBase>http://192.168.168.1:49152/</URLBase>"
	"</root>";

    ss.Seek(0);

    DescObserver dobs;
    DescriptionParser dp;
    unsigned int rc = dp.Parse(&ss, &dobs);
    assert(rc == 0);
    assert(dobs.url_base == "http://192.168.168.1:49152/");
    assert(dobs.root_device.type == "urn:schemas-upnp-org:device:MediaServer:1");
    assert(dobs.root_device.services.size() == 2);
//    TRACE << "sizeof(dp)=" << sizeof(dp) << "\n";
}

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

const SaxEvent events8[] = {
    { 'b', "wpl", NULL },
    { 'c', "X&Y", NULL },
    { 'e', "wpl", NULL },
    { 'f', NULL, NULL },
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
    { "<wpl>X&amp;Y</wpl>", events8 },
    { "<wpl><![CDATA[X&Y]]></wpl>", events8 }
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
	TRACE << "OnBegin(" << tag << ")\n";
	assert(m_events[m_count].event == 'b');
	assert(!strcmp(tag, m_events[m_count].arg1));
	++m_count;
	return 0;
    }
    unsigned int OnEnd(const char *tag)
    {
	CheckContent();
	TRACE << "OnEnd(" << tag << ")\n";
	assert(m_events[m_count].event == 'e');
	assert(!strcmp(tag, m_events[m_count].arg1));
	++m_count;
	return 0;
    }
    unsigned int OnContent(const char *tag)
    {
	TRACE << "OnContent(" << tag << ")\n";
	m_in_content = true;
	m_content += tag;
	return 0;
    }
    unsigned int OnAttribute(const char *tag, const char *value)
    {
	assert(!m_in_content);
	TRACE << "OnAttribute(" << tag << "," << value << ")\n";
	assert(m_events[m_count].event == 'a');
	assert(!strcmp(tag, m_events[m_count].arg1));
	assert(!strcmp(value, m_events[m_count].arg2));
	++m_count;
	return 0;
    }
};

void TestSax()
{
    for (unsigned int i=0; i<TESTS; ++i)
    {
	util::StringStream ss(saxtests[i].xml);
	size_t len = ss.str().length();

	for (size_t j=len; j; --j)
	{
//	    TRACE << "j=" << j << "\n";
	    util::DribbleStream dribble(&ss, j);

	    SaxTestObserver sto(saxtests[i].events);
	    xml::SaxParser saxp(&sto);
	    unsigned int rc = saxp.Parse(&dribble);
	    assert(rc == 0);
	}
    }
}

int main()
{
    TestSax();
    TestTableDriven();
    return 0;
}

#endif
