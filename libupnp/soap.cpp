#include "soap.h"
#include "libutil/trace.h"
#include "libutil/upnp.h"
#include "libutil/xmlescape.h"
#include <upnp/upnp.h>
#include <upnp/ixml.h>
#include <sstream>
#include <iomanip>
#include <string.h>

namespace upnp {
namespace soap {

struct Connection::Impl: public util::LibUPnPUser
{
    std::string url;
    std::string udn;
    const char *service;
};

Connection::Connection(const std::string& url,
		       const std::string& udn,
		       const char *service)
    : m_impl(new Impl)
{
    m_impl->url = url;
    m_impl->udn = udn;
    m_impl->service = service;
}

Connection::~Connection()
{
    delete m_impl;
}

static std::string IfPresent(Params *params, const char *what)
{
    Params::iterator i = params->find(what);
    if (i == params->end())
	return "";
    std::string result = "<" + i->first + ">" + util::XmlEscape(i->second)
	+ "</" + i->first + ">";
    params->erase(i);
    return result;
}

Params Connection::Action(const char *action_name,
			  const Params& params1)
{
    Params params = params1;
    Params result;

    std::ostringstream os;
    os << "<u:" << action_name << " xmlns:u=\"" << m_impl->service
       << "\">";

    /* Windows Media Connect needs its parameters in the right order
     * (and in fact SOAP1.1 para 7.1, rather disappointingly, says
     * that order is significant). So we carefully pluck out the ones
     * it needs first, first.
     */
    os << IfPresent(&params, "ObjectID");
    os << IfPresent(&params, "BrowseFlag");

    os << IfPresent(&params, "ContainerID");
    os << IfPresent(&params, "SearchCriteria");

    os << IfPresent(&params, "Filter");
    os << IfPresent(&params, "StartingIndex");
    os << IfPresent(&params, "RequestedCount");
    os << IfPresent(&params, "SortCriteria");

    os << IfPresent(&params, "InstanceID");

    for (Params::const_iterator i = params.begin(); i != params.end(); ++i)
	os << "<" << i->first << ">" << util::XmlEscape(i->second) << "</" << i->first << ">";
    os << "</u:" << action_name << ">";

    TRACE << os.str() << "\n";

    IXML_Document *request = ixmlParseBuffer(os.str().c_str());
    if (!request)
    {
	TRACE << "Can't ixmlparsebuffer\n";
	return result;
    }

    IXML_Document *response = NULL;
//    TRACE << "handle = " << m_impl->GetHandle() << "\n";
    int rc2 = UpnpSendAction(m_impl->GetHandle(),
			     m_impl->url.c_str(),
			     m_impl->service,
			     m_impl->udn.c_str(),
			     request,
			     &response);
    if (rc2 != 0)
    {
	TRACE << "SOAP failed, rc2 = " << rc2 << "\n";
    }

    if (response)
    {
	DOMString ds = ixmlPrintDocument(response);
	TRACE << ds << "\n";
	ixmlFreeDOMString(ds);
    }
    ixmlDocument_free(request);

    IXML_Node *child = ixmlNode_getFirstChild(&response->n);
    if (child)
    {
	IXML_NodeList *nl = ixmlNode_getChildNodes(child);
	
	unsigned int resultcount = ixmlNodeList_length(nl);
	TRACE << resultcount << " result(s)\n";

	for (unsigned int i=0; i<resultcount; ++i)
	{
	    IXML_Node *node2 = ixmlNodeList_item(nl, i);
	    if (node2)
	    {
		const DOMString ds = ixmlNode_getNodeName(node2);
		if (ds)
		{
		    IXML_Node *textnode = ixmlNode_getFirstChild(node2);
		    if (textnode)
		    {
			const DOMString ds2 = ixmlNode_getNodeValue(textnode);
			if (ds2)
			{
			    TRACE << ds << "=" << ds2 << "\n";
			    result[ds] = ds2;
			}
			else
			    TRACE << "no value\n";
		    }
		    else
			TRACE << "no textnode\n";
		}
		else
		    TRACE << "no name\n";
	    }
	    else
		TRACE << "no node\n";
	}
    }
    else
	TRACE << "no child\n";

    ixmlDocument_free(response);

    return result;
}

bool ParseBool(const std::string& s)
{
    if (s == "1") return true;
    if (s == "0") return false;
    if (!strcasecmp(s.c_str(),"true")) return true;
    if (!strcasecmp(s.c_str(),"false")) return false;
    if (!strcasecmp(s.c_str(),"yes")) return true;
    return false;
}

}; // namespace soap
}; // namespace upnp
