#ifndef HTTP_STREAM_H
#define HTTP_STREAM_H

#include "socket.h"
#include "stream.h"
#include <string>

namespace util {

namespace http {

class Stream: public util::SeekableStream
{
    IPEndPoint m_ipe;
    std::string m_host;
    std::string m_path;
    const char *m_extra_headers;
    const char *m_body;
    StreamSocketPtr m_socket;
    pos64 m_len;
    bool m_need_fetch;
    pos64 m_last_pos;

    Stream(const IPEndPoint& ipe, const std::string& host,
	   const std::string& path,
	   const char *extra_headers, const char *body);

public:
    ~Stream();

    typedef boost::intrusive_ptr< ::util::http::Stream> StreamPtr;

    static unsigned Create(StreamPtr*, const char *url,
			   const char *extra_headers = NULL,
			   const char *body = NULL);

    // Being a SeekableStream
    unsigned ReadAt(void *buffer, pos64 pos, size_t len, size_t *pread);
    unsigned WriteAt(const void *buffer, pos64 pos, size_t len, 
		     size_t *pwrote);
    pos64 GetLength();
    unsigned SetLength(pos64);
};

typedef boost::intrusive_ptr<Stream> StreamPtr;

} // namespace http

} // namespace util

#endif
