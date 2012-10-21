#include "config.h"
#include "soap.h"
#include "libutil/trace.h"
#include "libutil/xmlescape.h"
#include <sstream>
#include <iomanip>
#include <string.h>
#include <errno.h>
#include <boost/format.hpp>

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

Outbound::Outbound() {}
Outbound::~Outbound() {}

void Outbound::Add(const char *tag, const char *value)
{
    m_params.push_back(std::make_pair(tag,value));
}

void Outbound::Add(const char *tag, const std::string& value)
{
    m_params.push_back(std::make_pair(tag,value));
}

void Outbound::Add(const char *tag, int32_t value)
{
    m_params.push_back(std::make_pair(tag, 
				      (boost::format("%d") % value).str()));
}

void Outbound::Add(const char *tag, uint32_t value)
{
    m_params.push_back(std::make_pair(tag, 
				      (boost::format("%u") % value).str()));
}

std::string Outbound::CreateBody(const std::string& action_name,
				 const std::string& service_type) const
{
    std::ostringstream os;
    os << "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\""
	" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
	"<s:Body>"
	"<u:" << action_name << " xmlns:u=\"" << service_type
       << "\">";

    for (soap::Outbound::const_iterator i = begin(); i != end(); ++i)
	os << "<" << i->first << ">" << util::XmlEscape(i->second)
	   << "</" << i->first << ">";

    os << "</u:" << action_name << ">"
       << "</s:Body></s:Envelope>\n";

    return os.str();
}

Inbound::Inbound() {}
Inbound::~Inbound() {}

void Inbound::Get(std::string *ps, const char *tag)
{
    if (ps)
	*ps = m_params[tag];
}

void Inbound::Get(uint32_t *ps, const char *tag)
{
    if (ps)
	*ps = (uint32_t)strtoul(m_params[tag].c_str(), NULL, 10);
}

void Inbound::Get(int32_t *ps, const char *tag)
{
    if (ps)
	*ps = (int32_t)strtol(m_params[tag].c_str(), NULL, 10);
}

void Inbound::Get(uint8_t *ps, const char *tag)
{
    if (ps)
	*ps = (uint8_t)strtol(m_params[tag].c_str(), NULL, 10);
}

std::string Inbound::GetString(const char *tag) const
{
    params_t::const_iterator ci = m_params.find(tag);
    return ci == m_params.end() ? std::string() : ci->second;
}

uint32_t Inbound::GetUInt(const char *tag) const
{
    params_t::const_iterator ci = m_params.find(tag);
    return ci == m_params.end() ? 0 : (uint32_t)strtoul(ci->second.c_str(), NULL, 10);
}

int32_t Inbound::GetInt(const char *tag) const
{
    params_t::const_iterator ci = m_params.find(tag);
    return ci == m_params.end() ? 0 : (int32_t)strtol(ci->second.c_str(), NULL, 10);
}

bool Inbound::GetBool(const char *tag) const
{
    params_t::const_iterator ci = m_params.find(tag);
    return ci == m_params.end() ? false : ParseBool(ci->second);
}

uint32_t Inbound::GetEnum(const char *tag, const char *const *alternatives,
			  uint32_t n) const
{
    params_t::const_iterator ci = m_params.find(tag);
    if (ci == m_params.end())
	return n;
    for (uint32_t i=0; i<n; ++i)
    {
	if (!strcmp(ci->second.c_str(), alternatives[i]))
	    return i;
    }
    return n;
}

} // namespace soap
} // namespace upnp
