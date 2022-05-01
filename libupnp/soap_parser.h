#ifndef LIBUPNP_SOAP_PARSER_H
#define LIBUPNP_SOAP_PARSER_H

#include "libutil/xml.h"

/** Classes implementing UPnP.
 */
namespace upnp {

struct Data;

/** Classes implementing SOAP, the UPnP RPC protocol.
 */
namespace soap {

struct Params;

/** Parsing XML into a SOAP parameter structure
 *
 * SOAP is suitably symmetrical that this code works for both client and
 * server.
 */
class Parser: public xml::SaxParserObserver
{
    const upnp::Data *m_data;
    const unsigned char *m_expected;
    Params *m_result;

    unsigned int m_param;
    std::string m_value;

    enum { NO_PARAM = 255 };

    static int CaselessCompare(const void*, const void*);

public:
    Parser(const upnp::Data*, const unsigned char *expected, Params *result);

    // Being an xml::SaxParserObserver
    unsigned int OnBegin(const char *tag);
    unsigned int OnEnd(const char *tag);
    unsigned int OnContent(const char *content);
};

} // namespace soap
} // namespace upnp

#endif
