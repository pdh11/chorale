#include "config.h"
#include "description.h"
#include "libutil/trace.h"
#include "libutil/http_client.h"
#include <upnp/upnp.h>
#include <upnp/upnptools.h>
#include <upnp/ixml.h>

namespace upnp {

static std::string ParseXML(IXML_Document *xmldoc, const char *searchFor)
{
    std::string result;
    IXML_NodeList *nl = ixmlDocument_getElementsByTagName(xmldoc, searchFor);

    if (nl)
    {
	IXML_Node *node = ixmlNodeList_item(nl, 0);
	if (node)
	{
	    IXML_Node *textnode = ixmlNode_getFirstChild(node);
	    if (textnode)
	    {
		const DOMString ds = ixmlNode_getNodeValue(textnode);
		if (ds)
		{
//		    TRACE << "found '" << ds << "'\n";
		    result = ds;
		}
		else
		    TRACE << "no value\n";
	    }
	    else
		TRACE << "no textnode\n";
	}
	else
	    TRACE << "no node\n";
	
	ixmlNodeList_free(nl);
    }
    else
	TRACE << "no nodelist\n";

    return result;
}

static std::string GetChildNode(IXML_Node *node, const char *searchFor)
{
    std::string result;
    if (node->nodeType == eELEMENT_NODE)
    {
	IXML_Element *element = (IXML_Element*) node;
	IXML_NodeList *nl2 = ixmlElement_getElementsByTagName(element,
							      searchFor);
	if (nl2)
	{
	    IXML_Node *node2 = ixmlNodeList_item(nl2, 0);
	    if (node2)
	    {
		IXML_Node *textnode = ixmlNode_getFirstChild(node2);
		if (textnode)
		{
		    const DOMString ds2 = ixmlNode_getNodeValue(textnode);
		    if (ds2)
		    {
//			TRACE << "found2 '" << ds2 << "'\n";
			result = ds2;
		    }
		    else
			TRACE << "no value\n";
		}
		else
		    TRACE << "no textnode\n";
	    }
	    else
		TRACE << "no node\n";
	    
	    ixmlNodeList_free(nl2);
	}
	else
	    TRACE << "no nodelist 2\n";
    }
    else
	TRACE << "no element\n";
    return result;
}

unsigned Description::Fetch(const std::string& url)
{
    IXML_Document *xmldoc = NULL;
    TRACE << "downloading " << url << "\n";
    int rc = UpnpDownloadXmlDoc(url.c_str(), &xmldoc);
    TRACE << "UDXD " << rc << "\n";

    if (rc == 0 && xmldoc)
    {
//	DOMString ds = ixmlPrintDocument(xmldoc);
//	TRACE << ds << "\n";
//	ixmlFreeDOMString(ds);

	std::string base_url = ParseXML(xmldoc, "URLBase");
	if (base_url.empty())
	    base_url = url;

	m_udn = ParseXML(xmldoc, "UDN");
	m_friendly_name = ParseXML(xmldoc, "friendlyName");
	m_presentation_url = ParseXML(xmldoc, "presentationURL");

	if (!m_presentation_url.empty())
	    m_presentation_url = util::ResolveURL(base_url,
						  m_presentation_url);

	IXML_NodeList *nl = ixmlDocument_getElementsByTagName(xmldoc,
							      "service");
	if (nl)
	{
	    unsigned int servicecount = ixmlNodeList_length(nl);
//	    TRACE << servicecount << " service(s)\n";
	    for (unsigned int i=0; i<servicecount; ++i)
	    {
		IXML_Node *node = ixmlNodeList_item(nl, i);
		if (node)
		{
		    Service svc;
		    svc.type = GetChildNode(node, "serviceType");
		    svc.id = GetChildNode(node, "serviceId");
		    svc.control_url 
			= util::ResolveURL(base_url,
					   GetChildNode(node, "controlURL"));
		    svc.event_url
			= util::ResolveURL(base_url,
					   GetChildNode(node, "eventSubURL"));
		    svc.scpd_url
			= util::ResolveURL(base_url, 
					   GetChildNode(node, "SCPDURL"));
		    m_services[svc.type] = svc;
		}
		else
		    TRACE << "no node\n";
	    }

	    ixmlNodeList_free(nl);
	}
	else
	    TRACE << "no nodelist\n";
    }

    if (xmldoc)
    {
	ixmlDocument_free(xmldoc);
    }

    return 0;
}

}; // namespace upnp

