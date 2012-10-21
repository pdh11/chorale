#ifndef HTTP_STREAM_H
#define HTTP_STREAM_H

#include "socket.h"
#include "stream.h"
#include "counted_pointer.h"
#include <string>

namespace util {

namespace http {

class Client;

/** A SeekableStream representing a remote HTTP resource.
 *
 * Note that util::http::Connection is a stream (not seekable)
 * representing the result of a single HTTP transaction; http::Stream
 * is seekable and can and will end up using several HTTP transactions
 * internally to do its work.
 */
class Stream: public util::SeekableStream
{
    Client *m_client;
    IPEndPoint m_ipe;
    std::string m_host;
    std::string m_path;
    StreamSocketPtr m_socket;
    pos64 m_len;
    bool m_need_fetch;
    pos64 m_last_pos;

    Stream(Client *client, const IPEndPoint& ipe, const std::string& host,
	   const std::string& path);

public:
    ~Stream();

    typedef util::CountedPointer< ::util::http::Stream> StreamPtr;

    static unsigned Create(StreamPtr*, Client *client, const char *url);

    // Being a Pollable
    PollHandle GetHandle() { return m_socket->GetHandle(); }

    // Being a SeekableStream
    unsigned ReadAt(void *buffer, pos64 pos, size_t len, size_t *pread);
    unsigned WriteAt(const void *buffer, pos64 pos, size_t len, 
		     size_t *pwrote);
    pos64 GetLength();
    unsigned SetLength(pos64);
};

typedef util::CountedPointer<Stream> StreamPtr;

} // namespace http

} // namespace util

#endif
