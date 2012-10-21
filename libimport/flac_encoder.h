#ifndef LIBIMPORT_FLAC_ENCODER_H
#define LIBIMPORT_FLAC_ENCODER_H

#include "audio_encoder.h"
#include <stdint.h>

namespace import {

class FlacEncoder: public AudioEncoder
{
    util::Stream *m_output;
    void *m_flac;
    void *m_seektable;

public:

    // Being an AudioEncoder
    unsigned int Init(util::Stream *output, size_t sample_pairs);
    unsigned int Encode(const short *samples, size_t sample_pairs);
    unsigned int Finish();

    // Libflac callbacks
    int Write(const unsigned char *buffer, size_t bytes);
    int Seek(uint64_t pos);
    int Tell(uint64_t *ppos);
};

inline AudioEncoder *CreateFlacEncoder() { return new FlacEncoder; }

} // namespace import

#endif
