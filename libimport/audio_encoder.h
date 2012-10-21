#ifndef LIBIMPORT_AUDIO_ENCODER_H
#define LIBIMPORT_AUDIO_ENCODER_H

#include <string>
#include <stdlib.h>

namespace util { class Stream; }

namespace import {

class AudioEncoder
{
public:
    virtual ~AudioEncoder() {}

    virtual unsigned int Init(util::Stream *output,
			      size_t total_sample_pairs) = 0;

    /** Encode some audio and emit it to the output
     *
     * @param samples Raw 16-bit audio
     * @param sample_pairs Number of stereo sample pairs, i.e. sizeof(data)/4
     */
    virtual unsigned int Encode(const short *samples,
				size_t sample_pairs) = 0;
    
    virtual unsigned int Finish() = 0;

    virtual unsigned int PostFinish(const std::string&) { return 0; }
};

} // namespace import

#endif
