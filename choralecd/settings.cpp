/* settings.cpp
 *
 * This source code has been released by its author or authors into
 * the public domain, and all copyright is disclaimed.
 *
 * Note that *binaries* compiled from this source link against Qt, and
 * are thus subject to the licence under which you obtained Qt:
 * typically, the GPL.
 */
#include "settings.h"
#include <stdlib.h>

#define MP3ROOT "dirs/mp3"
#define FLACROOT "dirs/flac"
#define CDDBENABLE "cddb/enable"
#define PROXYENABLE "proxy/enable"
#define PROXYHOST "proxy/host"
#define PROXYPORT "proxy/port"

Settings::Settings()
    : m_qs(QSettings::UserScope, "pdh.utter.chaos.org.uk", "choralecd")
{
    m_qs.beginGroup("/choralecd");
}

std::string Settings::GetMP3Root() const
{
    std::string s = m_qs.readEntry(MP3ROOT).toUtf8().data();
    if (s.empty())
    {
	s = getenv("HOME");
	s += "/music/mp3";
    }
    return s;
}

void Settings::SetMP3Root(const std::string& s)
{
    m_qs.writeEntry(MP3ROOT, QString::fromUtf8(s.c_str()));
}

std::string Settings::GetFlacRoot() const
{
    std::string s = m_qs.readEntry(FLACROOT).toUtf8().data();
    if (s.empty())
    {
	s = getenv("HOME");
	s += "/music/flac";
    }
    return s;
}

void Settings::SetFlacRoot(const std::string& s)
{
    m_qs.writeEntry(FLACROOT, QString::fromUtf8(s.c_str()));
}

bool Settings::GetUseCDDB() const
{
    return m_qs.readBoolEntry(CDDBENABLE, false);
}

void Settings::SetUseCDDB(bool whether)
{
    m_qs.writeEntry(CDDBENABLE, whether);
}

bool Settings::GetUseHttpProxy() const
{
    return m_qs.readBoolEntry(PROXYENABLE, false);
}

void Settings::SetUseHttpProxy(bool whether)
{
    m_qs.writeEntry(PROXYENABLE, whether);
}

std::string Settings::GetHttpProxyHost() const
{
    return m_qs.readEntry(PROXYHOST).toUtf8().data();
}

void Settings::SetHttpProxyHost(const std::string& s)
{
    m_qs.writeEntry(PROXYHOST, QString::fromUtf8(s.c_str()));
}

unsigned short Settings::GetHttpProxyPort() const
{
    return (unsigned short)m_qs.readNumEntry(PROXYPORT);
}

void Settings::SetHttpProxyPort(unsigned short s)
{
    m_qs.writeEntry(PROXYPORT, s);
}
