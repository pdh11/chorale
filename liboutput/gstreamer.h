#ifndef OUTPUT_GSTREAMER_H
#define OUTPUT_GSTREAMER_H 1

#include "urlplayer.h"

namespace output {

/** Classes for local playback of media by driving %GStreamer.
 */
namespace gstreamer {

/** Implementation of URLPlayer in terms of %GStreamer.
 */
class URLPlayer: public output::URLPlayer
{
    class Impl;
    Impl *m_impl;

public:
    URLPlayer();
    ~URLPlayer();

    unsigned int SetURL(const std::string&, const std::string&);
    unsigned int SetNextURL(const std::string&, const std::string&);

    unsigned int SetPlayState(output::PlayState);

    void AddObserver(URLObserver*);
    void RemoveObserver(URLObserver*);
};

} // namespace gstreamer
} // namespace output

#endif
