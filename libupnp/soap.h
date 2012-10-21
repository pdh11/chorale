#ifndef LIBUPNP_SOAP_H
#define LIBUPNP_SOAP_H

#include <map>
#include <string>
#include <list>

/** Classes implementing UPnP.
 */
namespace upnp {

/** Classes implementing SOAP, the UPnP RPC protocol.
 */
namespace soap {

/** Inbound SOAP parameters (requests if we're a server, responses if
 * we're a client).
 */
class Inbound
{
    typedef std::map<std::string, std::string> params_t;
    params_t m_params;

public:
    Inbound();
    ~Inbound();

    void Set(const std::string& tag, const std::string& value)
    {
	m_params[tag] = value;
    }

    void Get(std::string*, const char*);
    void Get(uint32_t*,    const char*);
    void Get(int32_t*,     const char*);
    void Get(uint16_t*,    const char*);
    void Get(int16_t*,     const char*);
    void Get(uint8_t*,     const char*);
    void Get(int8_t*,      const char*);
    void Get(bool*,        const char*);

    std::string GetString(const char*) const;
    uint32_t GetUInt(const char*) const;
    int32_t GetInt(const char*) const;
    bool GetBool(const char*) const;

    uint32_t GetEnum(const char*, const char *const *alternatives,
		     uint32_t n) const;
};

/** Outbound SOAP parameters (requests if we're a client, responses if
 * we're a server).
 *
 * Windows Media Connect needs its parameters in the right order
 * (and in fact SOAP1.1 para 7.1, rather disappointingly, says
 * that order is significant). So we preserve parameter order here.
 */
class Outbound
{
    typedef std::list<std::pair<const char*, std::string> > params_t;
    params_t m_params;

public:
    Outbound();
    ~Outbound();

    void Add(const char*, const char *s);
    void Add(const char*, const std::string& s);
    void Add(const char*, int32_t i);
    void Add(const char*, uint32_t i);

    typedef params_t::const_iterator const_iterator;
    const_iterator begin() const { return m_params.begin(); }
    const_iterator end() const { return m_params.end(); }
};

/** A SOAP server.
 *
 * For the client implementation, see upnp::Client.
 */
class Server
{
public:
    virtual ~Server() {}

    virtual unsigned int OnAction(const char *action_name, 
				  const Inbound& params,
				  Outbound *result) = 0;
};

bool ParseBool(const std::string& s);

} // namespace soap
} // namespace upnp

#endif
