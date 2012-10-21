#ifndef OUTPUT_URLPLAYER_H
#define OUTPUT_URLPLAYER_H 1

#include <string>
#include "playstate.h"

namespace output {

/** Observer interface for URLPlayer.
 */
class URLObserver
{
public:
    virtual ~URLObserver() {}

    virtual void OnPlayState(output::PlayState) {}
    virtual void OnTimeCode(unsigned int /*sec*/) {}
    virtual void OnURL(const std::string&) {}
    virtual void OnError(unsigned int) {}
};

/** Abstract base class for anything able to play back media content by URL.
 */
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

} // namespace output

#endif
