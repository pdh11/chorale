#include "xml.h"
#include "libutil/trace.h"

namespace xml {

unsigned int BaseParser::Parse(util::StreamPtr, void*, const Data*)
{
    return 0;
}


}; // namespace xml

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

int main(int argc, char *argv[])
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

    return 0;
}

#endif
