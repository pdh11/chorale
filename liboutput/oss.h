#ifndef OUTPUT_OSS_H
#define OUTPUT_OSS_H 1

#include "libutil/poll.h"
#include "libutil/stream.h"

namespace output {

class Decoders
{
public:
    virtual ~Decoders() {}
    virtual util::SeekableStreamPtr NextPCMStream() = 0;
};

class OSSOutput: public util::Pollable
{
    int m_fd;
    char m_buffer[4608*4];
    int m_bufstart;
    int m_bufend;
    util::SeekableStreamPtr m_stream;
    util::PollerInterface *m_poller;
    Decoders *m_decoders;

public:
    OSSOutput(util::PollerInterface *poller, Decoders *decoders);

    void OpenOutput();

    void CloseOutput();

    unsigned OnActivity();
};

};

#endif
