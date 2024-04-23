#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>
#include <qsettings.h>

namespace choraleqt {

class Settings
{
    mutable QSettings m_qs;
    
public:
    Settings();

    std::string GetMP3Root() const;
    std::string GetFlacRoot() const;

    bool GetUseCDDB() const;

    bool GetUseHttpProxy() const;
    std::string GetHttpProxyHost() const;
    unsigned short GetHttpProxyPort() const;

    std::string GetDefaultDatabase() const;

    void SetMP3Root(const std::string&);
    void SetFlacRoot(const std::string&);

    void SetUseCDDB(bool);

    void SetUseHttpProxy(bool);
    void SetHttpProxyHost(const std::string&);
    void SetHttpProxyPort(unsigned short);

    void SetDefaultDatabase(const std::string&);
};

} // namespace choraleqt

#endif
