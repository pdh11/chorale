#ifndef HTTP_STREAM_H
#define HTTP_STREAM_H

#include "socket.h"
#include "stream.h"
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
    StreamSocket m_socket;
    uint64_t m_len;
    bool m_need_fetch;
    uint64_t m_last_pos;

    Stream(Client *client, const IPEndPoint& ipe, const std::string& host,
	   const std::string& path);

public:
    ~Stream();

    static unsigned Create(std::auto_ptr<util::Stream>*, Client *client,
			   const char *url);

    // Being a Pollable
    int GetHandle() { return m_socket.GetHandle(); }

    // Being a SeekableStream
    unsigned GetStreamFlags() const { return READABLE|SEEKABLE|POLLABLE; }
    unsigned ReadAt(void *buffer, uint64_t pos, size_t len, size_t *pread);
    unsigned WriteAt(const void *buffer, uint64_t pos, size_t len, 
		     size_t *pwrote);
    uint64_t GetLength();
    unsigned SetLength(uint64_t);
};

} // namespace http

} // namespace util

#endif
