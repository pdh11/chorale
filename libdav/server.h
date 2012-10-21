#ifndef LIBDAV_SERVER_H
#define LIBDAV_SERVER_H 1

#include "libutil/http_server.h"
#include <string>

/** Classes implementing WebDAV from RFC4918.
 */
namespace dav {

class Server: public util::http::ContentFactory
{
    std::string m_file_root;
    std::string m_page_root;

public:
    Server(const std::string& file_root,
	   const std::string& page_root);

    bool StreamForPath(const util::http::Request*, util::http::Response*);
};

} // namespace dav

#endif
