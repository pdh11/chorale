#ifndef HTTP_STREAM_H
#define HTTP_STREAM_H

#include "socket.h"
#include "stream.h"
#include <string>

namespace util {

class HTTPStream: public SeekableStream
{
    IPEndPoint m_ipe;
    std::string m_path;
    StreamSocket m_socket;
    pos64 m_len;
    bool m_need_fetch;
    StreamPtr m_stm;

    HTTPStream(const IPEndPoint&, const std::string& path);

public:
    ~HTTPStream();

    typedef boost::intrusive_ptr<HTTPStream> HTTPStreamPtr;

    static unsigned Create(HTTPStreamPtr*, const char *url);

    // Being a SeekableStream
    unsigned ReadAt(void *buffer, pos64 pos, size_t len, size_t *pread);
    unsigned WriteAt(const void *buffer, pos64 pos, size_t len, 
		     size_t *pwrote);
    pos64 GetLength();
    unsigned SetLength(pos64);
};

typedef boost::intrusive_ptr<HTTPStream> HTTPStreamPtr;

} // namespace util

#endif
