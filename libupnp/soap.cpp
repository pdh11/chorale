#include "config.h"
#include "soap.h"
#include "libutil/trace.h"
#include "libutil/upnp.h"
#include "libutil/xmlescape.h"
#include <sstream>
#include <iomanip>
#include <string.h>
#include <errno.h>
#include <boost/format.hpp>

#ifdef HAVE_UPNP

#include <upnp/upnp.h>
#include <upnp/ixml.h>

namespace upnp {
namespace soap {

static bool ParseBool(const std::string& s)
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

} // namespace soap
} // namespace upnp

#endif // HAVE_UPNP
