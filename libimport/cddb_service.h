#ifndef CDDB_SERVICE_H
#define CDDB_SERVICE_H

#include <string>
#include <vector>

#include "libutil/counted_object.h"
#include "audio_cd.h"

namespace import {

struct CDDBTrack
{
    std::string title;
    std::string artist;
    std::string comment;
};

struct CDDBFound
{
    unsigned int discid;
    unsigned int year;
    std::string title;
    std::string artist;
    std::string genre;
    std::string comment;
    std::vector<CDDBTrack> tracks;
};

struct CDDBLookup: public util::CountedObject<>
{
    std::vector<CDDBFound> discs;
};

typedef boost::intrusive_ptr<CDDBLookup> CDDBLookupPtr;

class CDDBService
{
    class Impl;
    Impl *m_impl;

public:
    CDDBService();
    ~CDDBService();

    void SetUseProxy(bool);
    void SetProxyHost(const std::string&);
    void SetProxyPort(unsigned short);

    CDDBLookupPtr Lookup(AudioCDPtr);
};

} // namespace import

#endif
