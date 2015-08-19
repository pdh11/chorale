#ifndef OUTPUT_GSTREAMER_H
#define OUTPUT_GSTREAMER_H 1

#include "urlplayer.h"

namespace output {

/** Classes for local playback of media by driving %GStreamer.
 */
namespace gstreamer {

/** Implementation of URLPlayer in terms of %GStreamer.
 */
class URLPlayer final: public output::URLPlayer
{
    class Impl;
    Impl *m_impl;

public:
    URLPlayer();
    ~URLPlayer();

    /** Pass in card==-1 to use the default output, or an ALSA
     * card/device pair.
     */
    unsigned int Init(int card = -1, int device = -1);

    unsigned int SetURL(const std::string&, const std::string&) override;
    unsigned int SetNextURL(const std::string&, const std::string&) override;

    unsigned int SetPlayState(output::PlayState) override;
    unsigned int Seek(unsigned int ms) override;

    void AddObserver(URLObserver*) override;
    void RemoveObserver(URLObserver*) override;
};

} // namespace gstreamer
} // namespace output

#endif
