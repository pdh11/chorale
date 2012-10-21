#include "soap.h"
#include "data.h"
#include "libutil/trace.h"
#include "libutil/printf.h"
#include "libutil/xmlescape.h"
#include <string.h>
#include <errno.h>
#include <stdio.h>

namespace upnp {
namespace soap {

bool ParseBool(const std::string& s)
{
    if (s == "1") return true;
    if (s == "0") return false;
    if (!strcasecmp(s.c_str(),"true")) return true;
    if (!strcasecmp(s.c_str(),"false")) return false;
    if (!strcasecmp(s.c_str(),"yes")) return true;
    return false;
}

unsigned int ParseEnum(const std::string& s, const char *const *alternatives,
		       unsigned int count)
{
    return ParseEnum(s.c_str(), alternatives, count);
}

unsigned int ParseEnum(const char *s, const char *const *alternatives,
		       unsigned int count)
{
    for (unsigned i=0; i<count; ++i)
    {
	if (!strcmp(s, alternatives[i]))
	    return i;
    }
    return count;
}

Params::Params()
{
}

Params::~Params()
{
}

std::string CreateBody(const upnp::Data *data, unsigned int action,
		       bool response, const char *service_type,
		       const Params& params)
{
    std::string s = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
	"<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\""
	" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n"
	" <s:Body>"
	"<u:";
    s += data->actions.alternatives[action];
    if (response)
	s += "Response";
    s += " xmlns:u=\"";
    s += service_type;
    s += "\">\r\n";

    const uint32_t *ptr32 = params.ints;
    const uint16_t *ptr16 = params.shorts;
    const uint8_t *ptr8 = params.bytes;
    const std::string *ptrstr = params.strings;
    
    for (const unsigned char *ptr = response ? data->action_results[action]
	     : data->action_args[action];
	 *ptr;
	 ++ptr)
    {
	unsigned param = *ptr - 48;
	uint8_t type = data->param_types[param];
	s += " <";
	s += data->params.alternatives[param];
	s += ">";
	if (type == Data::STRING)
	{
	    s += util::XmlEscape(*ptrstr++);
	}
	else if (type >= Data::ENUM)
	{
	    unsigned int which = type - Data::ENUM;
	    unsigned int n = *ptr32++;
	    s += data->enums[which].alternatives[n];
	}
	else if (type == Data::BOOL)
	{
	    s += *ptr8++ ? "1" : "0";
	}
	else
	{
	    // Numeric type
	    char buffer[16];
	    switch (type)
	    {
	    case Data::I8:
		sprintf(buffer, "%d", *ptr8++);
		break;
	    case Data::UI8:
		sprintf(buffer, "%u", *ptr8++);
		break;
	    case Data::I16:
		sprintf(buffer, "%d", *ptr16++);
		break;
	    case Data::UI16:
		sprintf(buffer, "%u", *ptr16++);
		break;
	    case Data::I32:
		sprintf(buffer, "%d", *ptr32++);
		break;
	    case Data::UI32:
		sprintf(buffer, "%u", *ptr32++);
		break;
	    }
	    s += buffer;
	}
	s += "</";
	s += data->params.alternatives[param];
	s += ">\r\n";
    }

    s += "</u:";
    s += data->actions.alternatives[action];
    if (response)
	s += "Response";
    s += ">\r\n</s:Body>\r\n</s:Envelope>\r\n";
    return s;
}

} // namespace soap
} // namespace upnp
