#ifndef OUTPUT_AVFOUNDATION_H
#define OUTPUT_AVFOUNDATION_H 1

#include "urlplayer.h"

namespace output {

/** Classes for local playback of media by driving Darwin's %AVFoundation.
 */
namespace avfoundation {

/** Implementation of URLPlayer in terms of Darwin's %AVFoundation.
 */
class URLPlayer: public output::URLPlayer
{
    class Impl;
    Impl *m_impl;

public:
    URLPlayer();
    ~URLPlayer();

    unsigned int Init();

    unsigned int SetURL(const std::string&, const std::string&);
    unsigned int SetNextURL(const std::string&, const std::string&);

    unsigned int SetPlayState(output::PlayState);
    unsigned int Seek(unsigned int ms);

    void AddObserver(URLObserver*);
    void RemoveObserver(URLObserver*);
};

} // namespace avframework
} // namespace output

#endif
