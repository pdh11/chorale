#ifndef LIBUTIL_HTTP_H
#define LIBUTIL_HTTP_H

#include <string>

namespace util {

/** Classes implementing HTTP/1.1
 */
namespace http {

std::string ResolveURL(const std::string& base, const std::string& link);

void ParseURL(const std::string& url, std::string *host,
	      std::string *path);

void ParseHost(const std::string& hostpart, unsigned short default_port,
	       std::string *hostname, unsigned short *port);

bool IsHttpURL(const char*);
inline bool IsHttpURL(const std::string& s) { return IsHttpURL(s.c_str()); }

} // namespace http

} // namespace util

#endif
