#include "config.h"
#include "description.h"
#include "libutil/trace.h"
#include "libutil/http_client.h"

#ifdef HAVE_UPNP

#include <upnp/upnp.h>
#include <upnp/ixml.h>

namespace upnp {

static std::string GetChildNode(IXML_Node *node, const char *searchFor)
{
    std::string result;
    if (node->nodeType == eELEMENT_NODE)
    {
	IXML_Element *element = (IXML_Element*) node;
	IXML_NodeList *nl2 = ixmlElement_getElementsByTagName(element,
							      const_cast<char *>(searchFor));
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

unsigned Description::Fetch(const std::string& url, const std::string& udn)
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

	IXML_Node *device_root = NULL;

	IXML_NodeList *udns = ixmlDocument_getElementsByTagName(xmldoc,
								    const_cast<char *>("UDN"));
	if (udns)
	{
	    unsigned int devicecount = ixmlNodeList_length(udns);
	    TRACE << devicecount << " devices\n";
	    for (unsigned int i=0; i<devicecount; ++i)
	    {
		IXML_Node *node = ixmlNodeList_item(udns, i);
		if (node)
		{
		    TRACE << "udn=" << node->firstChild->nodeValue << "\n";
		    IXML_Node *dev = node->parentNode;
		    TRACE << "dev->name=" << dev->nodeName << " value="
			  << dev->nodeValue << "\n";
		    if (node 
			&& node->firstChild 
			&& node->firstChild->nodeValue 
			&& udn == node->firstChild->nodeValue)
		    {
			device_root = node->parentNode;
			break;
		    }
		}
	    }
	    ixmlNodeList_free(udns);
	}

	if (!device_root)
	{
	    TRACE << "UDN " << udn << " not found on this device\n";
	}
	else
	{
	    m_udn = udn;
	    std::string base_url;
	
	    for (IXML_Node *node = device_root->firstChild;
		 node != NULL;
		 node = node->nextSibling)
	    {
		if (!strcmp(node->nodeName, "URLBase")
		    && node->firstChild
		    && node->firstChild->nodeValue)
		    base_url = node->firstChild->nodeValue;
		else if (!strcmp(node->nodeName, "friendlyName")
		    && node->firstChild
		    && node->firstChild->nodeValue)
		    m_friendly_name = node->firstChild->nodeValue;
		else if (!strcmp(node->nodeName, "presentationURL")
		    && node->firstChild
		    && node->firstChild->nodeValue)
		    m_presentation_url = node->firstChild->nodeValue;
	    }
		
	    if (base_url.empty())
		base_url = url;
	    
	    TRACE << "fn '" << m_friendly_name << "'\n";
	    
	    if (!m_presentation_url.empty())
		m_presentation_url = util::ResolveURL(base_url,
						      m_presentation_url);

	    for (IXML_Node *node = device_root->firstChild;
		 node != NULL;
		 node = node->nextSibling)
	    {
		if (!strcmp(node->nodeName, "serviceList")
		    && node->firstChild)
		{
		    for (IXML_Node *service = node->firstChild;
			 service != NULL;
			 service = service->nextSibling)
		    {
			ServiceData svc;
			svc.type = GetChildNode(service, "serviceType");
			svc.id = GetChildNode(service, "serviceId");
			svc.control_url 
			    = util::ResolveURL(base_url,
					       GetChildNode(service, "controlURL"));
			svc.event_url
			    = util::ResolveURL(base_url,
					       GetChildNode(service, "eventSubURL"));
			svc.scpd_url
			    = util::ResolveURL(base_url, 
					   GetChildNode(service, "SCPDURL"));
			TRACE << "service " << svc.type << "\n";
			m_services[svc.type] = svc;
		    }
		}
	    }
	}
    }

    if (xmldoc)
    {
	ixmlDocument_free(xmldoc);
    }

    return 0;
}

} // namespace upnp

#endif // HAVE_UPNP
