#include "soap_parser.h"
#include "soap.h"
#include "data.h"
#include <string.h>

namespace upnp {
namespace soap {

Parser::Parser(const upnp::Data *data, const unsigned char *expected,
	       Params *result)
    : m_data(data),
      m_expected(expected),
      m_result(result),
      m_param(NO_PARAM)
{
}

int Parser::CaselessCompare(const void *n1, const void *n2)
{
    return strcasecmp(*(const char**)n1, *(const char**)n2);
}

unsigned int Parser::OnBegin(const char *tag)
{
//    TRACE << "Tag: " << tag << "\n";
    m_value.clear();
    if (strchr(tag, ':'))
	return 0;

    m_param = m_data->params.Find(tag);
    if (m_param >= m_data->params.n)
    {
//	TRACE << "Unknown tag\n";
	m_param = NO_PARAM;
    }
    return 0;
}

unsigned int Parser::OnContent(const char *content)
{
    if (m_param != NO_PARAM)
	m_value += content;
    return 0;
}

unsigned int Parser::OnEnd(const char*)
{
    if (m_param != NO_PARAM)
    {
	uint32_t *ptr32 = m_result->ints;
	uint16_t *ptr16 = m_result->shorts;
	uint8_t *ptr8 = m_result->bytes;
	std::string *ptrstr = m_result->strings;

//	TRACE << m_data->params.alternatives[m_param] << ": " << m_value << "\n";

	for (const unsigned char *ptr = m_expected; *ptr; ++ptr)
	{
	    unsigned param = *ptr - 48;
	    uint8_t type = m_data->param_types[param];
	    if (param == m_param)
	    {
		if (type == Data::BOOL)
		    *ptr8 = ParseBool(m_value);
		else if (type == Data::STRING)
		    *ptrstr = m_value;
		else if (type >= Data::ENUM)
		{
		    unsigned int which = type - Data::ENUM;
		    *ptr32 = ParseEnum(m_value,
				       m_data->enums[which].alternatives,
				       m_data->enums[which].n);
		}
		else
		{
		    // Numeric type
		    int32_t i = (int32_t)strtol(m_value.c_str(), NULL, 10);
		    uint32_t ui = (uint32_t)strtoul(m_value.c_str(), NULL, 10);
		    switch (type)
		    {
		    case Data::I8:
			*ptr8 = (int8_t)i;
			break;
		    case Data::UI8:
			*ptr8 = (uint8_t)ui;
			break;
		    case Data::I16:
			*ptr16 = (int16_t)i;
			break;
		    case Data::UI16:
			*ptr16 = (uint16_t)ui;
			break;
		    case Data::I32:
			*ptr32 = i;
			break;
		    case Data::UI32:
			*ptr32 = ui;
			break;
		    }
		}
		m_param = NO_PARAM;
		m_value.clear();
		return 0;
	    }

	    // Not ours -- increment pointers
	    switch (type)
	    {
	    case Data::BOOL:
	    case Data::I8:
	    case Data::UI8:
		++ptr8;
		break;
	    case Data::I16:
	    case Data::UI16:
		++ptr16;
		break;
	    case Data::STRING:
		++ptrstr;
		break;
	    default: // 32-bit or enum
		++ptr32;
		break;
	    }
	}
    }
    return 0;
}

} // namespace soap
} // namespace upnp
