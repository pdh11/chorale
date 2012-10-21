#ifndef LIBIMPORT_MP3_ENCODER_H
#define LIBIMPORT_MP3_ENCODER_H

#include "audio_encoder.h"

namespace import {

class MP3Encoder: public AudioEncoder
{
    util::Stream *m_output;
    void *m_lame;

public:
    MP3Encoder();
    ~MP3Encoder();

    // Being an AudioEncoder
    unsigned int Init(util::Stream *output, size_t sample_pairs);
    unsigned int Encode(const short *samples, size_t sample_pairs);
    unsigned int Finish();
    unsigned int PostFinish(const std::string& filename);
};

inline AudioEncoder *CreateMP3Encoder() { return new MP3Encoder; }

} // namespace import

#endif
