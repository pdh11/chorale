#ifndef OUTPUT_URLPLAYER_H
#define OUTPUT_URLPLAYER_H 1

#include <string>
#include "playstate.h"

namespace output {

class URLObserver
{
public:
    virtual ~URLObserver() {}

    virtual void OnPlayState(output::PlayState) {}
    virtual void OnTimeCode(unsigned int /*sec*/) {}
    virtual void OnURL(const std::string&) {}
    virtual void OnError(unsigned int) {}
};

class URLPlayer
{
public:
    virtual ~URLPlayer() {}

    virtual unsigned int SetURL(const std::string& url,
				const std::string& metadataxml) = 0;
    virtual unsigned int SetNextURL(const std::string& url,
				    const std::string& metadataxml) = 0;

    virtual unsigned int SetPlayState(output::PlayState) = 0;

    virtual void AddObserver(URLObserver*) = 0;
    virtual void RemoveObserver(URLObserver*) = 0;
};

class GSTPlayer: public URLPlayer
{
    class Impl;
    Impl *m_impl;

public:
    GSTPlayer();
    ~GSTPlayer();

    unsigned int SetURL(const std::string&, const std::string&);
    unsigned int SetNextURL(const std::string&, const std::string&);

    unsigned int SetPlayState(output::PlayState);

    void AddObserver(URLObserver*);
    void RemoveObserver(URLObserver*);
};

}; // namespace output

#endif
