#ifndef LIBUTIL_HTTP_CLIENT_H
#define LIBUTIL_HTTP_CLIENT_H 1

#include <string>

namespace util {

class HttpClient
{
    std::string m_url;

public:
    HttpClient(const std::string& url);

    unsigned int FetchToString(std::string *presult);
};

std::string ResolveURL(const std::string& base, const std::string& link);

}; // namespace util

#endif
